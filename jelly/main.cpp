// jelly.cpp : Defines the entry point for the console application.
//

#include "JellyMessage.h"
#include "JellyServer.h"

class JellyCheckerProtocol
{
public:
	// List of Message IDs in this Protocol
	enum Codes
	{
		JELLY_MSG_CheckerHeal
	};
	// computed CRC of messages
	static const unsigned CRC = 0xCAFEBABE;
	static const char*    NAME;

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
	/*
	bool Put(DataChain* payload, int protocolType)
	{
		switch(protocolType)
		{
		default:
			return false;
		case JAM_PROTOCOL_BINARY_LITERAL:
		case JAM_PROTOCOL_BINARY_COMPRESSED:
			break;
		case JAM_PROTOCOL_TEST_JSON_COMPRESSED:
		case JAM_PROTOCOL_TEXT_JSON:
			break;
		}
	}
	*/
};
const char* JellyCheckerProtocol::NAME = "JellyCheckerProtocol";

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
	virtual bool Get(BinaryDecoder& decoder){ return true; }
	virtual bool Get(JSONDecoder&   decoder){ return true; }
	virtual bool Put(BinaryEncoder& encoder) const;
	virtual bool Put(JSONEncoder&   encoder) const
	{
		return true;
	}

	/**** DATA START ****/
	int value;
	/**** DATA STOP  ****/

	virtual unsigned short GetCode(){ return CODE; }
	virtual unsigned int   GetProtocolCRC(){ return JellyCheckerProtocol::CRC; }
};

unsigned short CheckerHeal::CODE   = JellyCheckerProtocol::JELLY_MSG_CheckerHeal;
unsigned int   CheckerHeal::CRC    = 0xABCDABCD;
const char*    CheckerHeal::NAME   = "CheckerHeal";

bool CheckerHeal::Put(BinaryEncoder& _encoder) const
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
	server.Start(1234);
	server.ConnectTo("127.0.0.1:1234");
	BoardServer bserver(&server);
	bserver.Configure();

	while(true)
	{
		Net::process();
	}


	Checker ch(&server);
	bserver.Add(&ch);
	CheckerHeal msg;
	msg.value = 100;

	server.Send( ch.GetID(), &msg);

	bserver.Idle();
	return 0;
}

