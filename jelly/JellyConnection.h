#ifndef __JELLY_CONNECTION_H__
#define __JELLY_CONNECTION_H__

#include <map>
#include "JellyTypes.h"
#include "JellyMessage.h"
#include "Net.h"

/**
	Remote Jelly server
 */
class JellyConnection
{
	friend JellyServer;
public:
	
	JellyConnection(Net::Socket_Id id,  JellyServer* server);

	/**
	 Send initial connection request
	 */
	void SendConnectRequest(BinaryEncoder* encoder);

	/**
	 Start listening for connections on a port
	 @param port  Port to list to
	 */
	bool Start(JELLY_U16 port);

	/**
	 Sends message to the server
	 */
	JELLY_RESULT Send(JellyMessage* msg);
	/**
	 Sends to a specific object
	 */
	JELLY_RESULT Send(ObjectID objectid, JellyMessage* msg){ return JELLY_OK; }

	void Receive(void* data, size_t len);

	/**
	 Get ChannelId for a given protocol CRC
	*/
	JELLY_S32 GetChannelId(JELLY_U32 crc);

	void AddCommonProtocol(const char* name, JELLY_U32 crc);

	JellyID GetID(){ return m_Id; }
protected:

	enum State
	{
		INIT_ID,
		INIT_PROTOCOL_NAME,
		INIT_PROTOCOL_CRC,
		CONNECTED,
	};

	JellyServer*   m_Server;	//!< link back to the server
	teDataChain    m_ReceiveChain;
	Net::Socket_Id m_Socket;
	State          m_State;
	JellyID        m_Id;

	JELLY_RESULT Send(teDataChain* chain);
	bool         ReceiveInit(teDataChain* chain);

	// from CRC to ChannelId
	typedef std::map<JELLY_U32, JELLY_U32> ProtocolMap;
	ProtocolMap m_KnownProtocols;

private:
};


#endif