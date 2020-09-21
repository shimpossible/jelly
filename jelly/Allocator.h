#pragma once

#include <stdint.h>

class Allocator
{
public:

	void* Allocate(size_t bytes)
	{
		return new char[bytes];
	}

	void Release(void* buffer)
	{
		delete[] buffer;
	}
};