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
private:
	struct SegmentInfo {
		int length;
		void* prevInMemory;
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

	/*void printAll() {
		cout << "freeSegments:" << freeSegments.size() << endl;
		for (set<void*>::iterator it = freeSegments.begin(); it != freeSegments.end(); ++it) {
			printf("%p,", *it);
		}
		cout << endl << "infoOfSegments:" << infoOfSegments.size() << endl;
		for (map<void*, SegmentInfo>::iterator it = infoOfSegments.begin(); it != infoOfSegments.end(); ++it) {
			printf("%p:%i,%p\n", it->first, it->second.length, it->second.prevInMemory);
		}
		cout << "lengthToFreePointers:" << lengthToFreePointers.size() << endl;
		for (map<int, set<void*>>::iterator it = lengthToFreePointers.begin(); it != lengthToFreePointers.end(); ++it) {
			cout << it->first << ":";
			for (set<void*>::iterator it1 = it->second.begin(); it1 != it->second.end(); ++it1) {
				cout << *it1 << ",";
			}
			cout << endl;
		}

		cout << endl;
	}*/

	void deepEraseFromLengthToFreePointers(void* ptr, int length);
	void deepEraseFromLengthToFreePointers(void* ptr, map<int, unordered_set<void*>>::iterator it);

public:
	CAllocator(int min_size, int max_size);
	~CAllocator();

	void* Alloc(int size);
	void Free(void* ptr);
};