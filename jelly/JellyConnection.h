#ifndef __JELLY_CONNECTION_H__
#define __JELLY_CONNECTION_H__

#include <map>
#include "JellyTypes.h"
#include "JellyMessage.h"
#include "JellyProtocol.h"
#include "Net.h"
#include "teBinaryEncoder.h"
#include "Allocator.h"

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
	void SendConnectRequest(teBinaryEncoder* encoder);

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
	JELLY_RESULT Send(ObjectID objectid, JellyMessage* msg)
	{
		// TODO: make this a message id
		return Send(msg);
	}

	void Receive(void* data, size_t len);

	/**
	 Get ChannelId for a given protocol CRC
	*/
	JELLY_S32 GetChannelId(JELLY_U32 crc);
	/**
	 Get CRC from Channel ID
	 */
	JELLY_RESULT GetCRC(JELLY_U32 channelId, JELLY_U32& crc);

	void AddCommonProtocol(const char* name, JELLY_U32 crc);

	JellyID GetID(){ return m_Id; }

	JellyMessage* CreateMessageObject(JELLY_U32 protocol_crc, JELLY_U16 msgId);
protected:

	enum State
	{
		INIT_ID,
		INIT_PROTOCOL_NAME,
		INIT_PROTOCOL_CRC,
		CONNECTED,
	};

	Allocator      m_Allocator;
	JellyServer*   m_Server;	//!< link back to the server

	char           m_Buffer[4096];
	teDataChain    m_ReceiveChain;
	Net::Socket_Id m_Socket;
	State          m_State;
	JellyID        m_Id;
	JellyLink      m_Link;

	JELLY_RESULT Send(teDataChain* chain);
	bool         ReceiveInit(teDataChain* chain);
	bool         ReceiveMsg(teDataChain* chain);

	struct ProtocolInfo
	{
		JELLY_U32      id;
		JellyProtocol* protocol;
	};
	typedef std::map<JELLY_U32, ProtocolInfo> ProtocolMap;  // CRC to Channel Id/Protocol

	ProtocolMap m_KnownProtocols;
	std::map<JELLY_U32, JELLY_U32> m_IdToCRC;

private:
};


#endif