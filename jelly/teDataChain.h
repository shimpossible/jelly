#pragma once

#include <stdint.h>
#include "JellyTypes.h"
#include <assert.h>

/**
  Chains blocks of memory together
 */
class teDataChain
{
public:

	const char* Buffer() {
		return m_Buffer;
	}

	size_t Length()
	{
		assert(m_Index >= m_Read);
		return m_Index - m_Read;
	}

	teDataChain()
	{
		m_Read = 0;
		m_Capacity = 1024;
		m_Index = 0;
	}
	~teDataChain()
	{
	}

	void Clear()
	{
		m_Index = 0;
		m_Read = 0;
	}

	bool AddTail(teDataChain* other)
	{
		return AddTail(&other->m_Buffer, other->m_Index);
	}

	/**
	 Add node to the END of the chain
	*/
	bool AddTail(const void* data, size_t len)
	{
		memcpy(&m_Buffer[m_Index], data, len);
		m_Index += len;
		return true;
	}

	bool AddHead(void* data, size_t len)
	{
		memmove(&m_Buffer[len], m_Buffer, len);
		memcpy(m_Buffer, data, len);

		return true;
	}

	 /**
	  Shift bytes off the front of the buffer
	  */
	bool Shift(void* dst, size_t len)
	{
		memcpy(dst, &m_Buffer[m_Read], len);
		m_Read += len;

		memmove(m_Buffer, &m_Buffer[m_Read], Length());
		m_Read -= len;
		m_Index -= len;
		return true;
	}

	void CopyTo(void* dst, size_t len)
	{
		memcpy(dst, &m_Buffer[m_Read], len);
	}

protected:

	char            m_Buffer[1024];
	size_t          m_Read;
	size_t          m_Index;
	size_t          m_Capacity;
};

