#ifndef __JELLY_ROUTE_CONFIG_H__
#define __JELLY_ROUTE_CONFIG_H__

#include "JellyTypes.h"
#include "JellyMessage.h"
#include "JellyMessageQueue.h"
#include <list>
#include <map>
#include "JellyProtocol.h"


/**
  If messages are to be queue, this is the object
  that will push the message into the queu, instead of routing
  them
 */
class JellyQueueDispatcher
{
public:
	JellyQueueDispatcher(JellyMessageQueue* q)
	{
		m_pQueue = q;
	}	
	void Dispatch(JellyLink& link, JellyMessage* msg)
	{
		m_pQueue->Add(link,msg);
	}

protected:
	JellyMessageQueue* m_pQueue;
};

class JellyRouteConfig
{
public:

	JellyRouteConfig();

	/**
	 *Route an incomming message to the correct Service
	 *dispatch method
	 */
	void Route(JellyLink& link, JellyMessage* msg, void* handler=0);

	/**
	  When ever a message in the PROTOCOL_T protocol is received, dispatch is called.  
	  @param handler the pHandler in the call to static JELLY_RESULT Dispatch(JellyMessage* pMessage,HANDLER_T* pHandler)
	 */
	template<typename PROTOCOL_T, typename HANDLER_T>
	void ConfigureInbound(HANDLER_T* handler, void (HANDLER_T::*dispatch)(JellyLink& link, JellyMessage* pMessage))
	{
		JellyProtocol* info = FindByCRC( PROTOCOL_T::CRC );
		if(info == nullptr)
		{
			info = new JellyProtocol(PROTOCOL_T::NAME, PROTOCOL_T::CRC, PROTOCOL_T::CreateMessage);
			m_Protocols.push_back(info);
		}
		info->AddHandler( new JellyServiceDispatchCommand<HANDLER_T>(handler, dispatch));
	}

	template<typename PROTOCOL_T, typename LOCK_T, typename HANDLER_T>
	void ConfigureInbound(HANDLER_T* handler, void (HANDLER_T::*dispatch)(JellyLink& link, JellyMessage* pMessage), LOCK_T& lock)
	{
		JellyProtocol* info = FindByCRC( PROTOCOL_T::CRC );
		if(info == nullptr)
		{
			info = new PROTOCOL_T();
			m_Protocols.push_back(info);
		}
		info->AddHandler( new JellyServiceLockableDispatchCommand<LOCK_T, HANDLER_T>(handler, dispatch, lock));
	}

	/**
	 * Message received for PROTOCOL_T are placed in a queue to be processed
	 * later
	 * @param handler   The handler object that we should queue for
	 * @param queue     Where to place the messages
	 */
	template< typename PROTOCOL_T, typename HANDLER_T>
	void ConfigureInbound(HANDLER_T* handler, JellyMessageQueue* queue)
	{
		JellyProtocol* info = FindByCRC( PROTOCOL_T::CRC );
		if(info == nullptr)
		{
			info = new PROTOCOL_T();
			m_Protocols.push_back(info);
		}
		info->AddQueue(queue, handler);
	}

	template<typename PROTOCOL_T>
	void ConfigureOutbound()
	{
		// Calling this will CREATE a new entry for the protocol
		JellyProtocol* info = FindByCRC( PROTOCOL_T::CRC );
		if(info == nullptr)
		{
			info = new JellyProtocol();
			m_Protocols.push_back(info);
		}
	}

	JellyProtocol* FindByCRC(JELLY_U32 id)
	{
		for(std::list<JellyProtocol*>::iterator it = m_Protocols.begin();
			it != m_Protocols.end();
			it++)
		{
			if( (*it)->id == id)
			{
				return *it;
			}
		}
		return 0;
	}

	JellyProtocol* FindByName(const char* name)
	{
		for(std::list<JellyProtocol*>::iterator it = m_Protocols.begin();
			it != m_Protocols.end();
			it++)
		{
			if( strcmp( (*it)->name,name) == 0)
			{
				return *it;
			}
		}
		return 0;
	}

	void ForEach( void (*fptr)(JellyProtocol* p, void* ctx), void* ctx)
	{
		for( std::list<JellyProtocol*>::iterator it = m_Protocols.begin();
			it != m_Protocols.end();
			it++)
		{
			fptr( *it, ctx);
		}
	}
protected:

	// List of protcols we know about
	std::list<JellyProtocol*> m_Protocols;
};


#endif //__JELLY_ROUTE_CONFIG_H__