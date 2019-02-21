#pragma once

#include "HeapManager.h"

#include <list>

const size_t ArenaSize = 4096 * 10000;
static CHeapManager customAllocator(0, ArenaSize);

class CCustomClass
{
public:
	CCustomClass();
	~CCustomClass();

	static void* operator new(size_t sz);
	static void operator delete(void* m);

private:
	int* buffer;
};