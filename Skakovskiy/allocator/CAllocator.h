#pragma once

#include "windows.h"

#include <iostream>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;
using std::set;
using std::unordered_set;
using std::map;
using std::unordered_map;
using std::vector;

class CAllocator {
public:
	CAllocator(int min_size, int max_size);
	~CAllocator();

	void* Alloc(int size);
	void Free(void* ptr);

private:
	struct SegmentInfo {
		int Length;
		void* PrevInMemory;
	};

	struct Segment {
		void* ptr;
		SegmentInfo info;
	};

	static const int PageSize = 4 * 1024;
	static const int RoundAlloc = 4;

	void* bufferPtr;
	void* endBufferPtr;

	unordered_set<void*> freeSegments;
	unordered_map<void*, SegmentInfo> infoOfSegments;
	map<int, unordered_set<void*>> lengthToFreePointers;

	static void* addInt(void* ptr, int delta);
	static int ptrsDiff(void* first, void* second);

	void* pageStartPointer(void* arg, bool isUpper);

	static void roundOff(int* arg, int precision);

	void deepEraseFromLengthToFreePointers(void* ptr, int length);
	void deepEraseFromLengthToFreePointers(void* ptr, map<int, unordered_set<void*>>::iterator it);
};