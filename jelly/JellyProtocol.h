#pragma once
#include "JellyDispatchCommand.h"
#include "JellyMessageQueue.h"
#include "JellyMessage.h"
#include "Allocator.h"
#include "teDataChain.h"

class JellyProtocol
{
	struct Tuple
	{
		JellyDispatchCommand* command;  // handler to dispatch the message
		JellyMessageQueue* queue;    // queue messages here if we can't dispatch
	};
	std::list<Tuple>          m_Handlers;
public:
	const char* name;           // text name of protocol
	JELLY_U32                 id;	          // id of protocol

	bool virtual Put(JellyMessage* msg, int p, teDataChain* chain) = 0;
	bool virtual Get(JellyMessage* msg, int p, teDataChain* chain) = 0;
	virtual JellyMessage* Create(JELLY_U32 id, Allocator& alloc) = 0;

	/**
	  Create new protocol metadata
	  @param name     text name of protocol
	  @param id       32bit ID of protocol
	 */
	JellyProtocol(const char* name, JELLY_U32 id)
	{
		this->name = name;
		this->id = id;
	}

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
			if (handler && it->command->TargetObject() == handler)
			{
				it->command->Dispatch(link, msg);
			}
			else
			{
				if (it->queue != 0)
				{
					it->queue->Add(link, msg);
				}
				else
				{
					it->command->Dispatch(link, msg);
				}
			}
		}
	}
};
