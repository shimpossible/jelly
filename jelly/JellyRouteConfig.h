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

template<typename T, class LOCK_T>
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

typedef void (*DispatchCallback)(JellyLink& link, JellyMessage* pMessage);

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
			ProtocolInfo* info = this->findByCRC( PROTOCOL_T::CRC );
		
			// TODO: check if this handler is already in the list of handlers

			// new command/queue tuple
			ProtocolTuple* tuple = new ProtocolTuple();
			tuple->command = new JellyServiceDispatchCommand<HANDLER_T>(handler, dispatch);
			tuple->queue   = nullptr;

			// add to list of handlers
			info->handlers.push_back( tuple);
	}
	template<typename PROTOCOL_T, typename LOCK_T, typename HANDLER_T>
	void ConfigureInbound(HANDLER_T* handler, void (HANDLER_T::*dispatch)(JellyLink& link, JellyMessage* pMessage), LOCK_T& lock)
	{
			ProtocolInfo* info = this->findByCRC( PROTOCOL_T::CRC );
		
			// TODO: check if this handler is already in the list of handlers

			// new command/queue tuple
			ProtocolTuple* tuple = new ProtocolTuple();
			tuple->command = new JellyServiceLockableDispatchCommand<HANDLER_T, LOCK_T>(handler, dispatch, lock);
			tuple->queue   = nullptr;

			// add to list of handlers
			info->handlers.push_back( tuple);
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
		ProtocolInfo* info = findByCRC( PROTOCOL_T::CRC );
		// find the matching handler
		for(std::list<ProtocolTuple*>::iterator it = info->handlers.begin();
			it != info->handlers.end();
			it++
			)
		{
			if( (*it)->command->TargetObject() == handler )
			{
				// we found the match, assign the queue to use
				(*it)->queue = queue;
				break;
			}
		}
	}

	template<typename PROTOCOL_T>
	void ConfigureOutbound()
	{
		// Calling this will CREATE a new entry for the protocol
		findByCRC( PROTOCOL_T::CRC );
	}

protected:

	template<class LOCK_T>
	struct Lock_Configure
	{
		static void ConfigureInbound(JellyRouteConfig* self)
		{
			ProtocolInfo* info = self->findByCRC( PROTOCOL_T::CRC );
		
			// TODO: check if this handler is already in the list of handlers

			// new command/queue tuple
			ProtocolTuple* tuple = new ProtocolTuple();
			tuple->command = new JellyServiceDispatchCommand<HANDLER_T,LOCK_T>(handler, dispatch);
			tuple->queue   = nullptr;

			// add to list of handlers
			info->handlers.push_back( tuple);
		}
	};

	// TODO: rename this into something better
	struct ProtocolTuple
	{
		JellyMessageQueue*    queue;
		JellyDispatchCommand* command;
	};

	struct ProtocolInfo
	{
		std::list<ProtocolTuple*> handlers;	// delegate for receiving
	};

	ProtocolInfo* findByCRC(JELLY_U32 crc)
	{
		ProtocolInfo* ret=0;
		std::map<JELLY_U32, ProtocolInfo*>::iterator match = knownProtocols.find(crc);
		if(match == knownProtocols.end() )
		{
			// not found
			ret = new ProtocolInfo();
			knownProtocols[crc] = ret;			
		}else
		{
			ret = match->second;
		}

		return ret;
	}
	// list of known protocols
	std::map<JELLY_U32, ProtocolInfo*> knownProtocols;
	//ProtocolInfo knownProtocols[256];
	//int  protocolCount;
};


#endif //__JELLY_ROUTE_CONFIG_H__