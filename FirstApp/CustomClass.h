#pragma once

#include "HeapManager.h"

#include <list>

const size_t ArenaSize = 4096 * 10000;
static CHeapManager customAllocator(0, ArenaSize);

class CCustomClass
{
public:
	CCustomClass() = default;
	~CCustomClass() = default;

	static void* operator new(size_t sz) 
	{
			return customAllocator.Alloc(sz);
	}
	static void operator delete(void* m) 
	{
		customAllocator.Free(m);
	}

private:
	int* buffer;
};