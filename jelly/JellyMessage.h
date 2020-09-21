#ifndef __JELLY_MESSAGE_H__
#define __JELLY_MESSAGE_H__


#include <atomic>
#include "JellyTypes.h"

/**
	Base message class.
	TODO: refcount
 */
class JellyMessage
{
	friend class JellyServer;
public:
	virtual ~JellyMessage(){}
	/**
	 * CRC of the protocol this Message belongs to
	 * This is to make sure the CODE is the right one for the message
	 */
	virtual unsigned int GetProtocolCRC()=0;

	/**
	  Object to send to
	*/
	ObjectID GetDestination(){return m_Destination;}

	virtual unsigned short GetCode()=0;

	virtual void AddRef() {}
	virtual void Release() {}
protected:

	JellyMessage()
		: m_Destination()
	{
	}

	// set while deserialize?
	ObjectID m_Destination;
};

#include "Allocator.h"
#include <assert.h>
template<typename T>
class RefCounted : public T
{
public:
	RefCounted(Allocator& alc)
		: m_Allocator(alc)
	{
		AddRef();
	}

	void AddRef()
	{
		m_Count++;
	}
	void Release() 
	{
		m_Count--;
		assert(m_Count >= 0);
		if (m_Count == 0)
		{
			m_Allocator.Release(this);
		}
	}
protected:

	std::atomic<int> m_Count;
private:

	Allocator& m_Allocator;
};


#endif