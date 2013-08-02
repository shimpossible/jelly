#ifndef __JELLY_ROUTE_CONFIG_H__
#define __JELLY_ROUTE_CONFIG_H__

#include "JellyTypes.h"
#include "JellyMessage.h"
#include "JellyMessageQueue.h"
#include <list>
#include <map>

class JellyDispatchCommand
{
public:
	virtual void Dispatch(JellyLink&, JellyMessage*)=0;
	void* TargetObject(){ return obj;}

protected:
	void* obj;
};


template<typename T>
class JellyServiceDispatchCommand : public JellyDispatchCommand
{
	void (T::*fptr)(JellyLink&, JellyMessage*);
public:
	JellyServiceDispatchCommand(T* o, void (T::*dispatch)(JellyLink&, JellyMessage*) )
	{
		obj = o;
		fptr = dispatch;
	}

	virtual void Dispatch(JellyLink& link, JellyMessage* msg)
	{
		T* t = (T*)obj;
		(t->*fptr)(link,msg);
	}
};

template<class LOCK_T,typename T>
class JellyServiceLockableDispatchCommand : public JellyDispatchCommand
{
	LOCK_T& m_Lock;
	void (T::*fptr)(JellyLink&, JellyMessage*);
public:
	JellyServiceLockableDispatchCommand(T* o, void (T::*dispatch)(JellyLink&, JellyMessage*), LOCK_T& lock )
		: m_Lock(lock)
	{
		
		obj = o;
		fptr = dispatch;
	}

	virtual void Dispatch(JellyLink& link, JellyMessage* msg)
	{
		m_Lock.Lock(msg,0);
		T* t = (T*)obj;
		(t->*fptr)(link,msg);
		m_Lock.Unlock(msg);
	}
};



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

class JellyProtocol
{
	struct Tuple
	{
	   JellyDispatchCommand*  command;  // handler to dispatch the message
	   JellyMessageQueue*     queue;    // queue messages here if we can't dispatch
	};
	std::list<Tuple>          m_Handlers;
public:
	typedef JellyMessage* (*CreatePtr)(JELLY_U16 msgId);
	const CreatePtr           create;
	const char*               name;      // ASCII name of protocol
	JELLY_U32                 crc;	     // CRC of protocol

	JellyProtocol(const char* name, JELLY_U32 crc, CreatePtr create)
		: create(create)
	{
		this->name = name;
		this->crc  = crc;
	}

	void AddHandler(JellyDispatchCommand* command)
	{
		Tuple t;
		t.command = command;
		t.queue   = 0;
		m_Handlers.push_back(t);
	}

	/**
	Add a message queue for the given handler
	 */
	void AddQueue(JellyMessageQueue* queue, void* handler)
	{
		for(std::list<Tuple>::iterator it = m_Handlers.begin();
			it != m_Handlers.end();
			it++)
		{
			if(it->command->TargetObject() == handler)
			{
				it->queue = queue;
				break;
			}
		}
	}

	/**
	 Dispatch the incomming message
	 */
	void operator()(JellyLink& link, JellyMessage* msg, void* handler)
	{
		for(auto it = m_Handlers.begin();
			it != m_Handlers.end();
			it++)
		{
			if(handler && it->command->TargetObject() == handler)
			{
				it->command->Dispatch(link,msg);
			}
			else
			{
				if(it->queue!=0)
				{
					it->queue->Add(link, msg);
				}else
				{
					it->command->Dispatch(link,msg);
				}
			}
		}
	}
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
			info = new JellyProtocol(PROTOCOL_T::NAME, PROTOCOL_T::CRC, PROTOCOL_T::CreateMessage);
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
			info = new JellyProtocol(PROTOCOL_T::NAME, PROTOCOL_T::CRC, PROTOCOL_T::CreateMessage);
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

	JellyProtocol* FindByCRC(JELLY_U32 crc)
	{
		for(std::list<JellyProtocol*>::iterator it = m_Protocols.begin();
			it != m_Protocols.end();
			it++)
		{
			if( (*it)->crc == crc)
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