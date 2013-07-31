#include "JellyConnection.h"
#include "JellyServer.h"

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
{
	m_Server = server;
	m_State = INIT_ID;
}

JELLY_S32 JellyConnection::GetChannelId(JELLY_U32 crc)
{
	JellyConnection::ProtocolMap::iterator it = m_KnownProtocols.find(crc);
	if(it != m_KnownProtocols.end())
	{
		return it->second;
	}
	// not found
	return -1;
}

static void __SendProtocol(JellyProtocol* proto, void* ctx)
{
	BinaryEncoder* enc = (BinaryEncoder*)ctx;
	int len = strlen(proto->name);
	enc->Put("length", len + 4);
	enc->Put("name", proto->name);
	enc->Put("crc", proto->crc);
}

void JellyConnection::SendConnectRequest(BinaryEncoder* encoder)
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
		JELLY_U8 channelId = found->second;
		JELLY_U8 msgType = JELLY_MESSAGE_SERVICE;
		JELLY_U16 msgId = msg->GetCode();

		// in the list of known protocols
		teDataChain payload;
		msg->Put( &payload );
		// payload includes the message id
		
		int msgSize = payload.Size();
		int sizeBytes = 1;
		if(msgSize >= 0x200000)    sizeBytes += 3;
		else if(msgSize >= 0x3FFF) sizeBytes += 2;
		else if(msgSize >= 0x80  ) sizeBytes += 1;

		// 1-3 bytes size
		JELLY_U32 encodedSize = Encode7Bit(msgSize + 4);
		teDataChain fullMessage;
		fullMessage.AddTail(&encodedSize, sizeBytes);

		// channel id
		// message type
		// message id
		fullMessage.AddTail(&channelId, sizeof(channelId));		
		fullMessage.AddTail(&msgType, sizeof(msgType));
		fullMessage.AddTail(&msgId, sizeof(msgId));

		// payload
		fullMessage.AddTail(&payload);
		
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
	// pump each link in the chain out the socket
	chain->ForEach( __send, &m_Socket );	
	return JELLY_OK;
}

void JellyConnection::Receive(void* data, size_t len)
{
	this->m_ReceiveChain.AddTail( data, len );

	bool loop=false;
	do
	{
		loop = false;
		switch(m_State)
		{
		case INIT_ID:
		case INIT_PROTOCOL_NAME:
		case INIT_PROTOCOL_CRC:
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
	bool loop = false;

	do
	{
		loop = false;
		// enough bytes for a size?
		if(chain->Size() >= 3)
		{
			JELLY_U8 tmp[3];
			chain->CopyTo(&tmp, sizeof(tmp));
			JELLY_U32 encodedBytes = 1;
			if(tmp[0] & 0x80) encodedBytes+=1;
			if(tmp[1] & 0x80) encodedBytes+=1;
			// should only encode 3 bytes max..
			//if(tmp[2] & 0x80) encodedBytes+=1;

			JELLY_U32 size = Decode7Bit(tmp);

			// enough for a full message?
			if(chain->Size() >= size+encodedBytes)
			{
				// shift off the 'size'
				chain->Shift(&tmp, encodedBytes);

				JELLY_U8  channel_id;
				JELLY_U8  message_type;
				JELLY_U16 message_id;

				JELLY_U32 payloadSize =  size - (sizeof(channel_id) + sizeof(message_type) + sizeof(message_id));
				chain->Shift(&channel_id, sizeof(channel_id));
				chain->Shift(&message_type, sizeof(message_type));
				chain->Shift(&message_id, sizeof(message_id));

				JELLY_U8 payload_tmp[1500];
				chain->Shift(&payload_tmp, payloadSize);
				loop = true;
			}

		}
	}while(loop);

	return loop;
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

	do
	{
		loop = false;


		if(m_State == INIT_ID)
		{
			if(chain->Size() >= 16)
			{
				// shift the 1st 16 bytes off into the ID
				chain->Shift( &m_Id, sizeof(m_Id) );
				m_State = INIT_PROTOCOL_NAME;
				loop = true;
			}
		}
	
		if (m_State == INIT_PROTOCOL_NAME)
		{
			JELLY_U32 numBytes=0;

			if(chain->Size() >= 4)
			{			
				chain->CopyTo(&numBytes,4);
				if(numBytes==0)
				{
					// eat the first 4 bytes
					chain->Shift(&numBytes,4);

					m_State = CONNECTED;
					printf("Connected\r\n");
					this->m_Server->m_Connections[ m_Id] = this;
					return true;
				}else
				{
					// enough for a Protocol?
					if(chain->Size() >= numBytes)
					{
						// TODO: just DROP the bytes..
						// eat the first 4 bytes
						chain->Shift(&numBytes,4);

						// parse the protocol

						int stringLen = numBytes - 4;
						JELLY_U32 crc;
						char* name = new char[ stringLen+1];
						name[stringLen] = 0;
						chain->Shift(name, stringLen);
						chain->Shift(&crc, 4);
						printf("name: %s : %08X\r\n", name, crc);

						AddCommonProtocol(name, crc);
						loop = true;
					}
				}
			}
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
		this->m_KnownProtocols[crc] = this->m_KnownProtocols.size();
	}
}