#include "CAllocator.h"

void* CAllocator::addInt(void* ptr, int delta) {
	return static_cast<void*>(static_cast<char*>(ptr) + delta);
}

int CAllocator::ptrsDiff(void* first, void* second) {
	return static_cast<char*>(first) - static_cast<char*>(second);
}

void* CAllocator::pageStartPointer(void* arg, bool isUpper) {
	int distFromStart = ptrsDiff(arg, bufferPtr);
	if (distFromStart % PageSize == 0) {
		return arg;
	}
	if (isUpper) {
		roundOff(&distFromStart, PageSize);
		return addInt(bufferPtr, distFromStart);
	}
	else {
		return addInt(arg, -(distFromStart % PageSize));
	}
}

void CAllocator::roundOff(int* arg, int precision) {
	if (*arg % precision != 0) {
		*arg += precision - *arg % precision;
	}
}

void CAllocator::deepEraseFromLengthToFreePointers(void* ptr, int length) {
	map<int, unordered_set<void*>>::iterator it = lengthToFreePointers.find(length);
	deepEraseFromLengthToFreePointers(ptr, it);
}

void CAllocator::deepEraseFromLengthToFreePointers(void* ptr, map<int, unordered_set<void*>>::iterator it) {
	if (it->second.size() == 1) {
		lengthToFreePointers.erase(it);
	}
	else {
		it->second.erase(ptr);
	}
}

CAllocator::CAllocator(int min_size, int max_size) {
	if (min_size <= 0 || max_size <= 0) {
		cerr << "min_size and max_size must be positive" << endl;
		return;
	}
	if (min_size >= max_size) {
		cerr << "min_size should be less than max_size" << endl;
		return;
	}

	min_size += RoundAlloc;
	roundOff(&min_size, PageSize);
	max_size += RoundAlloc;
	roundOff(&max_size, PageSize);

	bufferPtr = VirtualAlloc(nullptr, max_size, MEM_RESERVE, PAGE_READWRITE);
	if (bufferPtr != nullptr) {
		void* startPtr = VirtualAlloc(bufferPtr, min_size, MEM_COMMIT, PAGE_READWRITE);
		endBufferPtr = addInt(startPtr, max_size);
		infoOfSegments[startPtr] = { RoundAlloc, nullptr };
		if (max_size > min_size) {
			void* startOfFreeSegment = addInt(startPtr, RoundAlloc);
			freeSegments.insert(startOfFreeSegment);
			infoOfSegments[startOfFreeSegment] = { max_size - RoundAlloc, startPtr };
			lengthToFreePointers[max_size - RoundAlloc].insert(startOfFreeSegment);
		}
	} else {
		cerr << "Initially VirtualAlloc failed" << endl;
	}
}

CAllocator::~CAllocator() {
	if (freeSegments.size() != 1 || infoOfSegments[*freeSegments.begin()].PrevInMemory != bufferPtr) {
		cerr << "Memory leak detected" << endl;
	}
	VirtualFree(bufferPtr, ptrsDiff(endBufferPtr, bufferPtr), MEM_RELEASE);
}

void* CAllocator::Alloc(int size) {
	roundOff(&size, RoundAlloc);

	map<int, unordered_set<void*>>::iterator it = lengthToFreePointers.lower_bound(size);
	if (it == lengthToFreePointers.end()) {
		cerr << "Allocation failed: no free memory segment" << endl;
		return nullptr;
	}

	SegmentInfo oldSegmentInfo = infoOfSegments[*it->second.begin()];
	Segment resultSegment{
		*it->second.begin(),
		{
			size,
			oldSegmentInfo.PrevInMemory
		}
	};

	freeSegments.erase(resultSegment.ptr);

	if (resultSegment.info.Length != oldSegmentInfo.Length) {
		SegmentInfo nextSegmentInfo{
			oldSegmentInfo.Length - size,
			resultSegment.ptr
		};
		Segment nextSegment{
			addInt(resultSegment.ptr, size),
			nextSegmentInfo
		};

		freeSegments.insert(nextSegment.ptr);
		infoOfSegments[nextSegment.ptr] = nextSegmentInfo;
		infoOfSegments[resultSegment.ptr].Length = size;
		lengthToFreePointers[nextSegmentInfo.Length].insert(nextSegment.ptr);
	}
	deepEraseFromLengthToFreePointers(resultSegment.ptr, it);

	if (VirtualAlloc(resultSegment.ptr, size, MEM_COMMIT, PAGE_READWRITE) == nullptr) {
		cerr << "VirtualAlloc failed" << endl;
	}

	return resultSegment.ptr;
}

void CAllocator::Free(void* ptr) {
	unordered_map<void*, SegmentInfo>::iterator iterInInfoOfSegments = infoOfSegments.find(ptr);
	if (iterInInfoOfSegments == infoOfSegments.end()
		|| freeSegments.count(ptr) != 0
		) {
		cerr << "Free of not allocated pointer" << endl;
		return;
	}

	void* prev = iterInInfoOfSegments->second.PrevInMemory;
	void* next = addInt(ptr, iterInInfoOfSegments->second.Length);

	SegmentInfo prevSegmentInfo = infoOfSegments[prev];
	SegmentInfo nextSegmentInfo;
	if (next != endBufferPtr) {
		nextSegmentInfo = infoOfSegments[next];
	}

	bool isPrevFree = (freeSegments.count(prev) != 0);
	bool isNextFree = (freeSegments.count(next) != 0);
	int actualLength;
	if (isPrevFree && !isNextFree) {
		actualLength = iterInInfoOfSegments->second.Length + prevSegmentInfo.Length;

		infoOfSegments.erase(ptr);
		infoOfSegments[prev].Length = actualLength;
		if (next != endBufferPtr) {
			infoOfSegments[next].PrevInMemory = prev;
		}

		deepEraseFromLengthToFreePointers(prev, prevSegmentInfo.Length);
	} else if (!isPrevFree && isNextFree) {
		actualLength = iterInInfoOfSegments->second.Length + nextSegmentInfo.Length;

		freeSegments.erase(next);
		freeSegments.insert(ptr);

		void* nextNext = addInt(next, nextSegmentInfo.Length);
		if (nextNext != endBufferPtr) {
			infoOfSegments[nextNext].PrevInMemory = ptr;
		}
		infoOfSegments.erase(next);
		infoOfSegments[ptr].Length = actualLength;

		deepEraseFromLengthToFreePointers(next, nextSegmentInfo.Length);
	}
	else if (isPrevFree && isNextFree) {
		actualLength = iterInInfoOfSegments->second.Length + nextSegmentInfo.Length +
			prevSegmentInfo.Length;

		freeSegments.erase(next);

		void* nextNext = addInt(next, nextSegmentInfo.Length);
		if (nextNext != endBufferPtr) {
			infoOfSegments[nextNext].PrevInMemory = prev;
		}
		infoOfSegments.erase(ptr);
		infoOfSegments.erase(next);
		infoOfSegments[prev].Length = actualLength;

		deepEraseFromLengthToFreePointers(next, nextSegmentInfo.Length);
		deepEraseFromLengthToFreePointers(prev, prevSegmentInfo.Length);
	} else {
		actualLength = iterInInfoOfSegments->second.Length;

		freeSegments.insert(ptr);
	}

	void* startOfNewFreeSegment;
	if (isPrevFree) {
		startOfNewFreeSegment = prev;
	} else {
		startOfNewFreeSegment = ptr;
	}
	lengthToFreePointers[actualLength].insert(startOfNewFreeSegment);
	if (actualLength == PageSize) {
		void* startOfCurrentPage = pageStartPointer(startOfNewFreeSegment, false);
		if (startOfCurrentPage == startOfNewFreeSegment) {
			VirtualFree(startOfNewFreeSegment, PageSize, MEM_DECOMMIT);
		}
	} else if (actualLength > PageSize) {
		void* startOfFreeingSegment = max(
			pageStartPointer(startOfNewFreeSegment, true),
			pageStartPointer(ptr, false)
		);
		void* endOfFreeingSegment = min(
			pageStartPointer(addInt(startOfNewFreeSegment, actualLength), false),
			pageStartPointer(next, true)
		);
		if (startOfFreeingSegment != endOfFreeingSegment) {
			VirtualFree(startOfFreeingSegment, ptrsDiff(endOfFreeingSegment, startOfFreeingSegment), MEM_DECOMMIT);	
		}
	}
}