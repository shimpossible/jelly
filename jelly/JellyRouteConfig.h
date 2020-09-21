#ifndef __JELLY_ROUTE_CONFIG_H__
#define __JELLY_ROUTE_CONFIG_H__

#include "JellyTypes.h"
#include "JellyMessage.h"
#include "JellyMessageQueue.h"
#include <list>
#include <unordered_map>
#include "JellyProtocol.h"
#include <string>

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

template<typename K1, typename K2, typename V>
class MultiIndex
{
public:
	typedef std::unordered_map<std::tuple<K1, K2>, V> Map;
	typedef typename Map::iterator iterator;

	iterator begin() { return m_Map.begin(); }
	iterator end() { return m_Map.end(); }

	iterator find(K1& k)
	{
		std::tuple<K1, K2> key(k, K2());
		return m_Map.find(key);
	}
	iterator find(K2& k)
	{
		std::tuple<K1,K2> key(K1(), k);
		return m_Map.find(key);
	}
	void add(const K1& k1, K2& k2, V& v )
	{
		m_Map.emplace(std::tuple<K1,K2>(k1, k2), v);
		m_Map.emplace(std::tuple<K1, K2>(K1(), k2), v);
		m_Map.emplace(std::tuple<K1, K2>(k1, K2()), v);
	}
protected:

	typedef std::tuple<K1, K2> key_t;
	struct key_hash : public std::unary_function<key_t, std::size_t>
	{
		std::size_t operator()(const key_t& k) const
		{
			size_t t1 = std::hash<K1>()(std::get<0>(k));
			size_t t2 = std::hash<K2>()(std::get<1>(k));
			return t1 ^ t2;
		}
	};
	
	std::unordered_map<key_t, V, key_hash> m_Map;
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
	  When ever a message in the PROTOCOL_T protocol is received, Dispatch is called.  
	  @param handler the pHandler in the call to static JELLY_RESULT Dispatch(JellyMessage* pMessage,HANDLER_T* pHandler)
	 */
	template<typename PROTOCOL_T, typename HANDLER_T>
	void ConfigureInbound(HANDLER_T* handler, void (HANDLER_T::*dispatch)(JellyLink& link, JellyMessage* pMessage))
	{
		JellyProtocol* info = AddProtocol<PROTOCOL_T>(false);
		info->AddHandler( new JellyServiceDispatchCommand<HANDLER_T>(handler, dispatch));
	}

	template<typename PROTOCOL_T, typename LOCK_T, typename HANDLER_T>
	void ConfigureInbound(HANDLER_T* handler, void (HANDLER_T::*dispatch)(JellyLink& link, JellyMessage* pMessage), LOCK_T& lock)
	{
		JellyProtocol* info = AddProtocol<PROTOCOL_T>(true);
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
			info = new PROTOCOL_T();
			m_Protocols.push_back(info);
		}
	}

	JellyProtocol* FindByCRC(JELLY_U32 id)
	{
		iterator it = m_Protocols.find(id);
		if (it != m_Protocols.end())
		{
			return it->second.prot;
		}		
		return nullptr;
	}

	JellyProtocol* FindByName(const char* name)
	{
		std::string key(name);
		iterator it = m_Protocols.find(key);
		if (it != m_Protocols.end())
		{
			return it->second.prot;
		}
		return nullptr;
	}

	void ForEach( void (*fptr)(JellyProtocol* p, void* ctx), void* ctx)
	{
		for(iterator it = m_Protocols.begin();
			it != m_Protocols.end();
			it++)
		{
			fptr( it->second.prot, ctx);
		};
	}


	struct ProtocolRoute
	{
		JellyProtocol* prot;
		uint8_t        direction; // 1=incomming, 2=outgoing, 3=both
	};

	typedef MultiIndex<JELLY_U32, std::string, ProtocolRoute> ProtocolCollection;
	typedef ProtocolCollection::iterator iterator;

	iterator begin() { return m_Protocols.begin(); }
	iterator end()  { return m_Protocols.end(); }
protected:

	template<typename PROTOCOL_T>
	JellyProtocol* AddProtocol(bool in) 
	{
		JellyProtocol* info = FindByCRC(PROTOCOL_T::CRC);
		if (info == nullptr)
		{
			ProtocolRoute route;
			route.prot = info = new PROTOCOL_T();
			route.direction = in;
			m_Protocols.add(PROTOCOL_T::CRC, std::string(PROTOCOL_T::NAME), route);
		}
		return info;

	}
	// List of protcols we know about and their incoming/outgoing status
	ProtocolCollection m_Protocols;
};


#endif //__JELLY_ROUTE_CONFIG_H__