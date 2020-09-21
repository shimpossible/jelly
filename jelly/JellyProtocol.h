#pragma once
#include "JellyDispatchCommand.h"
#include "JellyMessageQueue.h"
#include "JellyMessage.h"
#include "Allocator.h"
#include "teDataChain.h"

/**
 Defines a group of messages
 */
class JellyProtocol
{
	struct Tuple
	{
		JellyDispatchCommand* command;  // handler to dispatch the message
		JellyMessageQueue*    queue;    // queue messages here if we can't dispatch
	};
	std::list<Tuple>          m_Handlers;
public:
	const char* name;           // text name of protocol
	const JELLY_U32   id;	    // id of protocol

	/**
	  Serialize a message 
	  @param  msg     Message to serialize
	  @param  p       type of encoding to use
	  @param  chain   Data Buffer to fill

	  @returns TRUE on success
	 */
	bool virtual Put(JellyMessage* msg, int p, teDataChain* chain) = 0;

	/**
	 Deserialize a message
	 @param  msg    Message to deserialize.  Must be a message in this protocol
	 @param  p      type of encoding to use
	 @param  chain  Data buffer to read

	 @returns TRUE on success
	 */
	bool virtual Get(JellyMessage* msg, int p, teDataChain* chain) = 0;

	/**
	  Create a message of given ID
	  @param id     ID of message to create
	  @param alloc  Allocates memory for message

	  @return ref counted object
	 */
	virtual JellyMessage* Create(JELLY_U32 id, Allocator& alloc) = 0;

	void AddHandler(JellyDispatchCommand* command)
	{
		Tuple t;
		t.command = command;
		t.queue = 0;
		m_Handlers.push_back(t);
	}

	/**
	Add a message queue for the given handler
	 */
	void AddQueue(JellyMessageQueue* queue, void* handler)
	{
		for (std::list<Tuple>::iterator it = m_Handlers.begin();
			it != m_Handlers.end();
			it++)
		{
			if (it->command->TargetObject() == handler)
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
		for (auto it = m_Handlers.begin();
			it != m_Handlers.end();
			it++)
		{
			// has a specific target, and this is not it...
			if (handler && it->command->TargetObject() != handler)
			{
				// go to next..
				continue;
			}

			// queue defined, place in queue to be dispatched later
			if (it->queue != 0)
			{
				msg->AddRef(); // increment so it doesn't get removed
				it->queue->Add(link, msg);
			}
			else
			{
				// Dispatch right now
				it->command->Dispatch(link, msg);
			}
		}
	}
protected:

	/**
    Create new protocol metadata
    @param name     text name of protocol
    @param id       32bit ID of protocol
    */
	JellyProtocol(const char* name, JELLY_U32 _id)
		: id(_id)
	{
		this->name = name;
	}
};
