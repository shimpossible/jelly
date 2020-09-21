#include "JellyConnection.h"
#include "JellyServer.h"

typedef struct MessageHeader
{
	JELLY_U8  channel_id;
	JELLY_U8  message_type;
	JELLY_U16 message_id;

	bool Put(teBinaryEncoder& encoder)
	{
		bool result = true;
		result = result && encoder.Put("channel", channel_id);
		result = result && encoder.Put("type",    message_type);
		result = result && encoder.Put("id",      message_id);
		return result;
	}

	bool Get(teBinaryDecoder& decoder)
	{
		bool result = true;
		result = result && decoder.Get("channel", channel_id);
		result = result && decoder.Get("type",    message_type);
		result = result && decoder.Get("id",      message_id);
		return result;
	}
} MessageHeader;

struct ProtocolHeader
{
	std::string name;
	JELLY_U32   crc;

	bool Put(teBinaryEncoder& encoder)
	{
		bool result = true;
		JELLY_U32 len = name.length() + 4;
		result = result && encoder.Put("len", len);
		result = result && encoder.Put("name", name.c_str(), name.length());
		result = result && encoder.Put("crc", crc);

		return result;
	}

	bool Get(teBinaryDecoder& decoder)
	{
		bool result = true;
		JELLY_U32 len = 0;
		result = result && decoder.Get("len", len);
		if (!result) return result;
		name.resize(len);
		if (len > 0)
		{
			result = result && decoder.Get("name", (char*)name.c_str(), len - 4);
			result = result && decoder.Get("crc", crc);
		}
		return result;
	}
};
/**
  7 bit integer encoding, LSByte first
 */
JELLY_U32 Encode7Bit(JELLY_U32 val)
{
	int ret=0;
	unsigned char* ptr = (unsigned char*)&ret;
	while(val > 0x80)
	{
		*ptr++ = (val|0x80) & 0xFF;
		val = val >> 7; // shift off the lower 7 bits
	}
	// val should be less than 0x80 at this point
	*ptr = (unsigned char) val;
	return ret;
}

JELLY_U32 Decode7Bit(JELLY_U8* val)
{
	int ret=0;

	// at most 4 bytes
	for(int i=0;i<4;i++)
	{
		ret += (*val &0x7F) << (i*7);
		if(*val < 0x80)
		{
			break;
		}
	}
	return ret;
}

JellyConnection::JellyConnection(Net::Socket_Id id, JellyServer* server)
	: m_Socket(id)
	, m_ReceiveChain(m_Buffer, sizeof(m_Buffer))
{
	m_Server = server;
	m_State = INIT_ID;
}

JELLY_S32 JellyConnection::GetChannelId(JELLY_U32 crc)
{
	JellyConnection::ProtocolMap::iterator it = m_KnownProtocols.find(crc);
	if(it != m_KnownProtocols.end())
	{
		return it->second.id;
	}
	// not found
	return -1;
}

JELLY_RESULT JellyConnection::GetCRC(JELLY_U32 channelId, JELLY_U32& crc)
{
	auto it = m_IdToCRC.find(channelId);
	if(it != m_IdToCRC.end())
	{
		crc =  it->second;
		return JELLY_OK;
	}
	// not found
	return JELLY_NOT_FOUND;
}

/**
	Send a single protocol over the line
 */
static void __SendProtocol(JellyProtocol* proto, void* ctx)
{
	teBinaryEncoder* enc = (teBinaryEncoder*)ctx;

	ProtocolHeader h;
	h.crc = proto->id;
	h.name = proto->name;
	h.Put(*enc);
}

void JellyConnection::SendConnectRequest(teBinaryEncoder* encoder)
{
	// send ID of THIS server
	encoder->Put("id", this->m_Server->GetID() );

	m_Server->GetRouteConfig().ForEach( __SendProtocol, encoder);

	encoder->Put("end", (JELLY_U32)0);
	//Net::send(m_Socket, "Hello", 6,0);
}

/**
 *  Encode a message into the byte stream
 *
 *   Size          1-3 bytes
 *   ...
 *   ...
 *   Channel ID    1 byte
 *   Message Type  1 byte
 *   Message ID    2 bytes
 *   ...
 *   Payload
 *   ...
 *
 *
 */
JELLY_RESULT JellyConnection::Send(JellyMessage* msg)
{
	JELLY_U32 pcrc = msg->GetProtocolCRC();
	
	ProtocolMap::iterator found = this->m_KnownProtocols.find(pcrc);
	if(found != m_KnownProtocols.end() )
	{
		printf("Sending message %08x\r\n", msg->GetCode() );
		MessageHeader header;
		// TODO: if we need to increase the channel_id size..
		// this is where
		header.channel_id   = (JELLY_U8)found->second.id;
		header.message_type = JELLY_MESSAGE_SERVICE;
		header.message_id   = msg->GetCode();

		// in the list of known protocols
		char buffer[4096];

		// allow 16 bytes of header
		teDataChain payload(&buffer[16], sizeof(buffer)-16);
		{
			found->second.protocol->Put(msg,0, &payload);
		}
		// payload includes the message id
		
		int msgSize = payload.Length();
		int sizeBytes = 1;
		if(msgSize >= 0x200000)    sizeBytes += 3;
		else if(msgSize >= 0x3FFF) sizeBytes += 2;
		else if(msgSize >= 0x80  ) sizeBytes += 1;

		// 1-3 bytes size
		JELLY_U32 encodedSize = Encode7Bit(msgSize + 4);

		teDataChain fullMessage(buffer, 16);
		teBinaryEncoder enc(&fullMessage);
		enc.Put("size",  &encodedSize, sizeBytes); // TODO: make a special 7BitEncoded type
		enc.Put("header", header);

		// payload
		fullMessage.Add(payload);
		
		return Send(&fullMessage);
	}

	return JELLY_NOT_FOUND;

}

static void __send(void* data, size_t len, void* ctx)
{
	Net::Socket_Id id = *(Net::Socket_Id*)ctx;
	Net::send(id, (const char*) data, len,0);
}
JELLY_RESULT JellyConnection::Send(teDataChain* chain)
{
	Net::send(m_Socket, chain, 0);
	return JELLY_OK;
}

void JellyConnection::Receive(void* data, size_t len)
{
	// read all the bytes, reset the pointers
	if (m_ReceiveChain.Remaining() == 0) m_ReceiveChain.Clear();

	// Append the latest data
	this->m_ReceiveChain.Write( data, len );

	bool loop=false;
	do
	{
		loop = false;
		switch(m_State)
		{
		case INIT_ID:	          // ID of remote 
		case INIT_PROTOCOL_NAME:  // name of protocol
		case INIT_PROTOCOL_CRC:   // crc of protocol
			loop=ReceiveInit(&m_ReceiveChain);
			// TODO: should loop once more if there are more bytes left
			break;
		case CONNECTED:
			loop = ReceiveMsg(&m_ReceiveChain);
			break;
		}
	}while(loop);
}

bool JellyConnection::ReceiveMsg(teDataChain* chain)
{
	do
	{
		char* prev = chain->buffer.read_head;

		// enough bytes for a size?
		if (chain->Remaining() < 3)
		{
			// not enough, exit loop
			return false;
		}
		teBinaryDecoder dec(chain);
		UInt7BitEncoded size = 0;

		if (!dec.Get("size", size)) break;

		// enough for a full message?
		if (chain->Remaining() < size)
		{
			// not enough, reset pointer and exit loop
			chain->buffer.read_head = prev;
			return false;
		}
		MessageHeader header;
		JELLY_U32 payloadSize =  size - sizeof(header);
		bool st = header.Get(dec);

		printf("rcv: %d %d %08x\n",
			st,
			header.channel_id,
			header.message_id
			);
		//JELLY_U8 payload_tmp[1500];
		//chain->Shift(&payload_tmp[ sizeof(JellyMessage)], payloadSize);
		// need to convert from RAW bytes to a JellyMessage object (placement new)
		// based on the CRC
		JELLY_U32 proto_crc;

		// Convert from Channel Id to Protocol
		if(GetCRC(header.channel_id, proto_crc) == JELLY_OK)
		{
			ProtocolMap::iterator it = this->m_KnownProtocols.find(proto_crc);
			// this shouldn't ever be NOT OK.. but just to be safe
			JellyMessage* msg = CreateMessageObject(proto_crc, header.message_id);

			// TODO: improve this.  We haev the protocol here.. but
			// RouteConfig has to look it up again during Route
			it->second.protocol->Get(msg, 0, chain);

			// Route to everyone that registered to receive the message
			(*it->second.protocol)(m_Link, msg, 0);

			msg->Release();
		}

		// TODO: drop remaining bytes...

	}while(true);

	return false;
}

JellyMessage* JellyConnection::CreateMessageObject(JELLY_U32 id, JELLY_U16 msgId)
{
	// TODO: use a pool of messages..
	JellyProtocol* p = m_Server->GetRouteConfig().FindByCRC(id);
	JellyMessage* m = p->Create(msgId, m_Allocator);	
	return m;
}

/**
  First thing we do on connect is send an ID and Protocols

  +-----------------------+
  |           ID          |
  |   128 bit             |
  |                       |
  |                       |
  +-----------------------+
  | length 32bits         |
  +-----------------------+
  | NAME                  |
  +-----------------------+
  | 32 bit CRC            |
  +-----------------------+
  | ..................... |
  +-----------------------+
  | END marker (NULL      |
  +-----------------------+


 */
bool JellyConnection::ReceiveInit(teDataChain* chain)
{
	bool loop=false;

	teBinaryDecoder dec(chain);
	do
	{
		loop = false;


		if(m_State == INIT_ID)
		{
			// ID is 16 bytes long
			if(chain->Length() >= 16)
			{
				// shift the 1st 16 bytes off into the ID
				dec.Get("id", m_Id);
				m_State = INIT_PROTOCOL_NAME;
				loop = true;
			}
		}
	
		if (m_State == INIT_PROTOCOL_NAME)
		{
			ProtocolHeader h;
			if (!h.Get(dec))
			{
				break;
			}

			if (h.name.length() == 0)
			{
				m_State = CONNECTED;
				printf("Connected\r\n");
				this->m_Server->m_Connections[m_Id] = this;
				return true;
			}
			// parse the protocol

			printf("Matching protocol name:%s id:%08X\r\n", h.name.c_str(), h.crc);

			AddCommonProtocol(h.name.c_str(), h.crc);
			loop = true;
		}	
	}while(loop);

	return loop;
}

void JellyConnection::AddCommonProtocol(const char* name, JELLY_U32 crc)
{
	// look through our list of protocols
	JellyProtocol* proto = m_Server->GetRouteConfig().FindByName(name);
	// unknown / supported protocol
	if(proto == 0) return;
	
	// only add if not already in the list
	ProtocolMap::iterator it =  this->m_KnownProtocols.find(crc);
	if(it == m_KnownProtocols.end() )
	{
		JELLY_U32 id = this->m_KnownProtocols.size();
		this->m_KnownProtocols[crc] = ProtocolInfo{ id, proto };
		this->m_IdToCRC[ id]        = crc;
	}
}


bool BinaryGet(teBinaryDecoder& dec, JellyMessage& t)
{
	
	return true;
}