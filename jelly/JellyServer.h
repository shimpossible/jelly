#ifndef __JELLY_SERVER_H__
#define __JELLY_SERVER_H__
#include "JellyTypes.h"
#include "JellyRouteConfig.h"
#include "JellyMessage.h"


/**
	Remote Jelly server
 */
class JellyClient
{
public:
	JELLY_RESULT Send(JellyID objectid, JellyMessage* msg);
};
class JellyServer
{
public:
	/**
		Sends message to specific object on remote server
		@param server Remote service to send to
		@param object id of object on remote server
		@param msg    message to send
	 */
	JELLY_RESULT Send(JellyID server, JellyID objectId, JellyMessage* msg);
	
	/**
		Sends message to service/service type
		@param server  id of server
		@param msg     message to send
	 */
	JELLY_RESULT Broadcast(JellyID server, JellyMessage* msg);
	// send a message to a specific object
	JELLY_RESULT Send(ObjectID obj, JellyMessage* msg)
	{
		msg->destination = obj;
		m_RouteConfig.Route(m_Link, msg);

		return JELLY_OK;
	}

	/**
		Dispatch messages found in the queue
		@param queue   Queue of messages that were received in the past
		@param handler Handler to dispatch messages to
	 */
	void ProcessQueue(JellyMessageQueue* queue, void* handler)
	{
		JellyLink link;
		JellyMessage* msg;
		queue->Dequeue(link, msg);
		m_RouteConfig.Route(link,msg, handler);
	}

	JellyRouteConfig& GetRouteConfig(){ return m_RouteConfig; }

protected:
	JellyRouteConfig m_RouteConfig;
	JellyLink        m_Link; // link to ourself
};

/*
message_type = service | object | negotiation | ping
message_id   = 0 to 0xFFFF

json message?
{
    "_msgID":10,
    "type":6, 
    "error":0,
    "desc":{
        "m_id":"T2R00S40.00E14815726P10987H127.0.0.1:14001",
		//  T 2                 type?
		//  R 00
		//  S 40.00             32bits?
		//  E 14815726          24 bits?  32bits?
		//  P 10987             16 bits?
		//  H 127.0.0.1:14001   48bits
        "m_host":"127.0.0.1",
        "m_partitionID":0,
        "m_configID":0,
        "m_buildNum":0,
        "m_type":40,
        "m_subType":0
    }
}

*/
#endif //__JELLY_SERVER_H__