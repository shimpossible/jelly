// jelly.cpp : Defines the entry point for the console application.
//

#include "JellyMessage.h"
#include "JellyServer.h"
#include "JellyProtocol.h"

class CheckerHeal : public JellyMessage
{
public:
	static unsigned int   CRC;
	static unsigned short CODE;
	static const char*    NAME;

	CheckerHeal() 
	{
		value = 0;  // default value
	}
	// Message Decoders
	virtual bool Get(teBinaryDecoder& decoder)
	{
		decoder.Get("value", value);
		
		return true; 
	
	}
	virtual bool Get(JSONDecoder&   decoder){ return true; }
	virtual bool Put(teBinaryEncoder& encoder) const;
	virtual bool Put(JSONEncoder&   encoder) const
	{
		return true;
	}

	/**** DATA START ****/
	JELLY_U32 value;
	/**** DATA STOP  ****/

	virtual unsigned short GetCode(){ return CODE; }
	virtual unsigned int   GetProtocolCRC();
};

class JellyCheckerProtocol : public JellyProtocol
{
public:
	// List of Message IDs in this Protocol
	enum Codes
	{
		JELLY_MSG_CheckerHeal = 0x00001234
	};

	// computed CRC of messages
	static const unsigned CRC = 0xCAFEBABE;
	static const char*    NAME;

	JellyCheckerProtocol()
		: JellyProtocol(NAME, CRC)
	{
	}

	virtual JellyMessage* Create(JELLY_U32 id, Allocator& alloc)
	{
		JellyMessage* result = 0;
		void* buffer = 0;
		switch (id)
		{
		case JELLY_MSG_CheckerHeal:			
			buffer = alloc.Allocate(sizeof(CheckerHeal));
			result = new (buffer) CheckerHeal();
			break;
		default:
			// no match
			break;
		}

		return result;
	}

	bool Put(JellyMessage* msg, int protocolType, teDataChain* chain)
	{
		switch (protocolType)
		{
		default:
		case 0:
			teBinaryEncoder enc(chain);
			return Put(msg, enc);
		}
	}
	bool Put(JellyMessage* msg, teBinaryEncoder& enc)
	{
		JELLY_U32 code = msg->GetCode();
		switch (code)
		{
		case JELLY_MSG_CheckerHeal:
			return enc.Put("msg", *(CheckerHeal*)msg);
			break;
		}

		return false;
	}

	bool Get(JellyMessage* msg, int protocolType, teDataChain* chain)
	{
		switch (protocolType)
		{
		default:
		case 0:
			teBinaryDecoder enc(chain);
			return Get(msg, enc);
		}
	}
	bool Get(JellyMessage* msg, teBinaryDecoder& dec)
	{
		JELLY_U32 code = msg->GetCode();
		switch (code)
		{
		case JELLY_MSG_CheckerHeal:
			return dec.Get("msg", *(CheckerHeal*)msg);
			break;
		}

		return false;
	}

	//NOTICE: Generated code DO NOT EDIT
	template<typename HANDLER_T>
	static JELLY_RESULT Dispatch(JellyLink& link, JellyMessage* pMessage,HANDLER_T* pHandler)
	{
		/** NOTE: JellyLink& link arg was missing from slides */
		JELLY_RESULT result = JELLY_OK;
		switch(pMessage->GetCode() )
		{
		case JELLY_MSG_CheckerHeal:
			result = pHandler->CheckerHealHandler(link, (CheckerHeal*)pMessage);
			break;
		}
		return result;
	}
	/* ?? what was this part for?
	bool Put(DataChain* payload, int protocolType)
	{
		switch(protocolType)
		{
		default:
			return false;
		case JAM_PROTOCOL_BINARY_LITERAL:
		case JAM_PROTOCOL_BINARY_COMPRESSED:
			teRawBinaryEncoder encoder(payload);
			return Put(encoder);
		case JAM_PROTOCOL_TEST_JSON_COMPRESSED:
		case JAM_PROTOCOL_TEXT_JSON:
			break;
		}
	}
	*/
};

const char* JellyCheckerProtocol::NAME = "JellyCheckerProtocol";


unsigned short CheckerHeal::CODE   = JellyCheckerProtocol::JELLY_MSG_CheckerHeal;
unsigned int   CheckerHeal::CRC    = 0xABCDABCD;
const char*    CheckerHeal::NAME   = "CheckerHeal";
unsigned int   CheckerHeal::GetProtocolCRC(){ return JellyCheckerProtocol::CRC; }

bool CheckerHeal::Put(teBinaryEncoder& _encoder) const
{
	_encoder.BeginMessage(CODE, NAME);
	_encoder.Put("value", value);
	_encoder.EndMessage(CODE,NAME);

	return true;
}
/*
bool CheckerHeal::Put(teDataChain* pPayload)
{
	switch(protocolType)
	{
		default:
			return false;
		case JELLY_PROTOCOL_BINARY_LITERAL:
		{
			teRawBinaryEncoder encoder(pPayload);
			return Put(encoder);
		}
		case JELLY_PROTOCOL_TEXT_JSON:
		{
			teRawJSONEncoder encoder(pPayload);
			return Put(encoder);
		}
	}
}
*/

/**
	An object to send to and receive from
 */
class Checker 
{
public:
	JellyID m_ServerId;
	JellyServer* m_pServer;
	ObjectID m_id;

	Checker(JellyServer* server)
	{
		m_pServer = server;
		m_id = ObjectID::Create();
	}
	JellyID GetServer()
	{
		return m_ServerId;
	}

	ObjectID GetID()
	{
		return m_id;
	}

	
	/*
	not sure what this is, but it only shows here
    JamID destination = GetRouter()->GetCreditManagerID();
    GetRouter()->Send(destination, &msg);
	JamRouter* GetRouter() {}
	*/
	
	/*
	void HealChecker(ObjectID toHeal, unsigned int amount)
	{
		CheckerHeal msg;
		msg.value = amount;
		m_pServer->Send(toHeal, &msg);
	}
	*/

	JELLY_RESULT CheckerHealHandler(JellyLink& link, CheckerHeal* pMessage)
	{
		printf("heal for %d\r\n", pMessage->value );
		return JELLY_OK;
	}
};

class MatchmakerLockPolicy
{
public:
    void Lock(JellyMessage *msg, JellyMessageQueue **ppQueue)
    {
		printf("aquire lock\r\n");
    }
    void Unlock(JellyMessage *msg)
	{
		printf("release lock");
	}
};


class BoardServer
{
public:
	JellyServer* m_pJellyServer;
	JellyMessageQueue m_messageQueue;
	MatchmakerLockPolicy   m_LockPolicy;
	BoardServer(JellyServer* server)
	{
		m_pJellyServer = server;
	}
	/*
	void Send(Checker* pChecker, JellyMessage* pMessage)
	{
		m_pJellyServer->Send(pChecker->GetServer(),
			pChecker->GetID(),
			pMessage);
	}
	*/

	void Configure()
	{
		JellyRouteConfig& routeConfig = m_pJellyServer->GetRouteConfig();
		routeConfig.ConfigureInbound<JellyCheckerProtocol>(this, &BoardServer::CheckerDispatch, m_LockPolicy);
		routeConfig.ConfigureInbound<JellyCheckerProtocol>(this, &m_messageQueue);
	}

	void Idle()
	{
		m_pJellyServer->ProcessQueue(&m_messageQueue, this);
	}

	void CheckerDispatch(JellyLink& link, JellyMessage* pMessage)
	{
		ObjectID destId = pMessage->GetDestination();
		Checker* pChecker = GetCheckerObject(destId);
		// why do we queue this here, if we dispatch it right afterwards?
		//pChecker->QueueMessage(pMessage);
		switch(pMessage->GetProtocolCRC())
		{
		case JellyCheckerProtocol::CRC:
			//NOTE: link was missing from slides
			JellyCheckerProtocol::Dispatch<Checker>(link, pMessage, pChecker);
			break;
		}
	}

	Checker* checkers;
	void Add(Checker* ch)
	{
		checkers = ch;
	}
	Checker* GetCheckerObject(ObjectID id)
	{
		// TODO: find checker by id
		return checkers;
	}
};

#include "Net.h"

int main(int argc, char* argv[])
{
	Net::init();
	/*
	while(true)
	{
		Net::process();
	}
	*/
	
	JellyServer server;
	BoardServer bserver(&server);
	bserver.Configure();

	// start AFTER everything is configured
	server.Start(1234);
	server.ConnectTo("127.0.0.1:1234");

	for(int i=0;i<100;i++)
	{
		Net::process();
	}


	Checker ch(&server);
	bserver.Add(&ch);
	CheckerHeal msg;
	msg.value = 100;

	while(true)
	{
		Net::process();

		bserver.Idle();

		server.Send(ch.GetID(), &msg);
	}	
	return 0;
}

