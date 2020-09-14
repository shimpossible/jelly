#ifndef __JELLY_MESSAGE_QUEUE_H__
#define __JELLY_MESSAGE_QUEUE_H__

#include "JellyTypes.h"
#include <queue>

class JellyMessageQueue
{
	// TODO: make this queue not allocate new objects..
public:
	void Add(JellyLink& link, JellyMessage* msg)
	{
		Tuple* t = new Tuple();
		t->link = link;
		t->msg = msg;
		m_Queue.push(t);
	}

	void Dequeue(JellyLink& link, JellyMessage*& msg)
	{
		Tuple* t = m_Queue.front();
		link  = t->link;
		msg   = t->msg;

		delete t;
		m_Queue.pop();
	}

	bool Empty()
	{
		return m_Queue.empty();
	}
protected:
	struct Tuple
	{
		JellyLink     link;
		JellyMessage* msg;
	};
	std::queue<Tuple*> m_Queue;
};

#endif // __JELLY_MESSAGE_QUEUE_H__