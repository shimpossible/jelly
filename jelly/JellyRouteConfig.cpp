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
	// TODO: calling this creates a NEW object, we dont want to do that
	JellyProtocol* proto = FindByCRC(crc);
		
	(*proto)(link,msg, handler);

	/*
	{
		// call all handlers
		for(std::list<ProtocolTuple*>::iterator itr = info->handlers.begin();
			itr != info->handlers.end();
			itr++
			)
		{
			// filtering by handler
			if(handler !=0 && (*itr)->command->TargetObject() != handler) 
				continue;

			// if we have a queue assigned use that
			if(handler == 0 && 
			   (*itr)->queue != nullptr
			  )
			{
				(*itr)->queue->Add(link, msg);
			}
			else
			{
				// no queue, just route it
				(*itr)->command->Dispatch(link, msg);
			}
		}
	}
	*/
}
