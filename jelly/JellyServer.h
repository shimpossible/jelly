#ifndef __JELLY_SERVER_H__
#define __JELLY_SERVER_H__
#include "JellyTypes.h"
#include "JellyRouteConfig.h"
#include "JellyMessage.h"
#include "JellyConnection.h"
#include "Net.h"

#include <unordered_map>



class JellyServer
{
	friend JellyConnection;
public:
	JellyServer();

	bool Start(JELLY_U16 port);
	bool ConnectTo(const char* address);
	/**
		Sends message to specific object on remote server
		@param server Remote service to send to
		@param object id of object on remote server
		@param msg    message to send
	 */
	JELLY_RESULT Send(JellyID server, ObjectID objectId, JellyMessage* msg);

	/**
	 Sends to a specfic service/server
	 */
	JELLY_RESULT Send(JellyID server, JellyMessage* msg);
	
	/**
		Sends message to service/service type
		@param type    type of service to send to
		@param msg     message to send
	 */
	JELLY_RESULT Broadcast(int type, JellyMessage* msg);

	/**
	  Sends a message to a specific object. This sends to all
	  services that support the protocol
	*/
	JELLY_RESULT Send(ObjectID obj, JellyMessage* msg);

	/**
		Dispatch messages found in the queue
		@param queue   Queue of messages that were received in the past
		@param handler Handler to dispatch messages to
	 */
	void ProcessQueue(JellyMessageQueue* queue, void* handler);

	JellyRouteConfig& GetRouteConfig(){ return m_RouteConfig; }

	JellyID GetID(){ return m_ID; }

protected:
	JellyRouteConfig m_RouteConfig;
	JellyLink        m_Link; // link to ourself
	JellyID          m_ID;

	typedef std::unordered_map<JellyID, JellyConnection*> ConnectionMap;
	typedef std::unordered_map<Net::Socket_Id, JellyConnection*> ClientSocketMap;
	typedef std::map<Net::Socket_Id, JellyServer*>               ServerSocketMap;

	ConnectionMap              m_Connections;	
	static ClientSocketMap     s_Clients;
	static ServerSocketMap     s_KnownConnections;

	static Net::ops s_NetOps;
	static Net::ops s_ClientNetOps;
	
	static void net_state_changed(Net::Socket_Id id, Net::State state);
	static void net_accepted(Net::Socket_Id id, Net::Socket_Id other, Net::Address& address);
	static void net_received(Net::Socket_Id id, void* data, size_t len);

	static JellyServer* Find(Net::Socket_Id id);
	static JellyConnection* FindClient(Net::Socket_Id id);
	/**
	A new client that hasn't negotiated protocols yet..
	 */
	static void AddPending(Net::Socket_Id id,JellyConnection* client);
};

#endif //__JELLY_SERVER_H__