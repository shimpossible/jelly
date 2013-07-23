#ifndef __JELLY_MESSAGE_QUEUE_H__
#define __JELLY_MESSAGE_QUEUE_H__

#include "JellyTypes.h"
#include <queue>

class JellyMessageQueue
{
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
		link  = m_Queue.front()->link;
		msg   = m_Queue.front()->msg;
		m_Queue.pop();
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