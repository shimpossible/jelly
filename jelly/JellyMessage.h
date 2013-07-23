#ifndef __JELLY_MESSAGE_H__
#define __JELLY_MESSAGE_H__

#include <queue>
#include <list>
#include "JellyMessageQueue.h"
#include "JellyRouteConfig.h"

/**
	Base message class.
	TODO: refcount
 */
class JellyMessage
{
	friend class JellyServer;
public:
	virtual ~JellyMessage(){}

	/**
	 * CRC of the protocol this Message belongs to
	 * This is to make sure the CODE is the right one for the message
	 */
	virtual unsigned int GetProtocolCRC()=0;

	/**
	  Object to send to
	*/
	ObjectID GetDestination(){return destination;}

	virtual unsigned short GetCode()=0;

protected:

	JellyMessage()
	{
		destination = 0;
	}
	// set while deserialize?
	ObjectID destination;
};


#endif