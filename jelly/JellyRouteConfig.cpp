#include "JellyRouteConfig.h"


JellyRouteConfig::JellyRouteConfig()
{
}

/**
	*Route an incomming message to the correct Service
	*dispatch method
	* 
	*/
void JellyRouteConfig::Route(JellyLink& link, JellyMessage* msg, void* handler)
{
	unsigned int crc    = msg->GetProtocolCRC();
	JellyProtocol* proto = FindByCRC(crc);

	// Have the protocol Dispatch
	(*proto)(link,msg, handler);
}
