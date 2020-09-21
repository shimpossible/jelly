#pragma once

#include <stdint.h>
#include "JellyTypes.h"
#include <assert.h>

struct teDataBuffer
{
	char*  start;    //!< start of buffer
	char*  end;      //!< End of buffer

	char* read_head;  //!< Where to read from
	char* write_head; //!< Where to write to
};

/**
  Chains blocks of memory together
 */
class teDataChain
{
public:
	teDataChain* Next() { return next; }
	teDataChain(void* data, size_t len)	
	{
		buffer.start      = (char*)data;
		buffer.read_head  = (char*)data;
		buffer.write_head = (char*)data;
		buffer.end = buffer.start + len;
		next = nullptr;
	}
	~teDataChain()
	{
	}

	size_t Remaining()
	{
		size_t result = (buffer.write_head - buffer.read_head);
		if (next) result += next->Remaining();
		return result;
	}
	size_t Length()
	{
		size_t result = (buffer.write_head - buffer.start);
		if (next) result += next->Length();
		return result;
	}
	void Clear()
	{
		buffer.write_head = buffer.start;
		buffer.read_head  = buffer.start;

		if (next) next->Clear();
	}

	void Add(teDataChain& other)
	{
		next = &other;
	}
	bool Write(const void* data, size_t len)
	{
		// bytes left in buffer?
		size_t bytes = buffer.end - buffer.write_head;
		// clip to length
		if (bytes > len) bytes = len;

		// room in buffer?
		if (bytes>0)
		{
			memcpy(buffer.write_head, data, bytes);
			buffer.write_head += bytes;
		}

		len -= bytes;

		// any remaining data goes to next in chain
		if (len && next) return next->Write((const char*)data + bytes, len);

		// otherwise, return TRUE if all data was pushed
		return len == 0;
	}

	bool Read(void* dst, size_t len)
	{
		// bytes left in buffer?
		size_t bytes = buffer.write_head - buffer.read_head;
		// clip to length
		if (bytes > len) bytes = len;

		// room in buffer?
		if (bytes > 0)
		{
			memcpy(dst, buffer.read_head, bytes);
			buffer.read_head += bytes;
		}

		len -= bytes;

		// any remaining data goes to next in chain
		if (len && next) return next->Write((const char*)dst + bytes, len);

		// otherwise, return TRUE if all data was pushed
		return len == 0;
	}

	teDataBuffer buffer;
protected:

	teDataChain* next;
};

