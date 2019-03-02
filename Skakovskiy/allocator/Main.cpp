#include <iostream>
#include <random>
#include <ctime>
#include <vector>

#include "windows.h"
#include "CAllocator.h"

using std::vector;
using std::cout;
using std::endl;

const int AllocatorCapacity = 1024 * 1024 * 500;
CAllocator alloc(4, AllocatorCapacity); // 500 mb
HANDLE hHeap;

void fillRequestData(int TestSize, vector<bool> &toAlloc, vector<int> &allocSize, int min, int max) {
	std::random_device rd1, rd2;
	std::mt19937 gen1(rd1());
	std::mt19937 gen2(rd2());
	std::uniform_int_distribution<> dis1(1, 2);
	std::uniform_int_distribution<> dis2(min, max);

	for (int i = 0; i < TestSize; ++i) {
		toAlloc[i] = (dis1(rd1) % 2 == 0);
		allocSize[i] = dis2(rd2);
	}
}

void makeHHeap() {
	hHeap = HeapCreate(0, 0, AllocatorCapacity);
}

void testRandAllocFree(int TestSize, const vector<bool> &toAlloc, const vector<int> &allocSize) {
	set<void*> ptrs;
	int startTime = clock();

	for (int i = 0; i < TestSize; ++i) {
		if (ptrs.size() == 0) {
			ptrs.insert(alloc.Alloc(allocSize[i]));
		}
		else {
			if (toAlloc[i] == 1) {
				ptrs.insert(alloc.Alloc(allocSize[i]));
			}
			else {
				alloc.Free(*ptrs.begin());
				ptrs.erase(ptrs.begin());
			}
		}
	}
	printf("testRandAllocFree size: %i\nMyAllocator: %i\n", TestSize, clock() - startTime);

	while (!ptrs.empty()) {
		alloc.Free(*ptrs.begin());
		ptrs.erase(ptrs.begin());
	}
	
	startTime = clock();
	for (int i = 0; i < TestSize; ++i) {
		if (ptrs.size() == 0) {
			ptrs.insert(HeapAlloc(hHeap, 0, allocSize[i]));
		}
		else {
			if (toAlloc[i] == 1) {
				ptrs.insert(HeapAlloc(hHeap, 0, allocSize[i]));
			} else {
				HeapFree(hHeap, 0, *(ptrs.begin()));
				ptrs.erase(ptrs.begin());
			}
		}
	}
	printf("HHeap: %i\n\n", clock() - startTime);
}

void testSequentialAlloc(int TestSize, int allocSize, bool isStraightDirection) {
	vector<void*> ptrsVector(TestSize, nullptr);

	int startTime = clock();
	for (int i = 0; i < TestSize; ++i) {
		ptrsVector[i] = alloc.Alloc(allocSize);
	}
	int middleTime = clock();
	for (int i = 0; i < TestSize; ++i) {
		alloc.Free(ptrsVector[isStraightDirection ? i : TestSize - 1 - i]);
	}
	printf(
		"testRandAllocFree size: %i, isStraightDirection: %i, allocSize: %i\n"
		"MyAllocator: %i - alloc %i - free\n", 
		TestSize, isStraightDirection ?1:0, allocSize,
		middleTime - startTime, clock() - middleTime);
	
	startTime = clock();
	for (int i = 0; i < TestSize; ++i) {
		ptrsVector[i] = HeapAlloc(hHeap, 0, allocSize);
	}
	middleTime = clock();
	for (int i = 0; i < TestSize; ++i) {
		HeapFree(hHeap, 0, ptrsVector[isStraightDirection ? i : TestSize - 1 - i]);
	}
	printf("HHeap: %i - alloc %i - free\n\n",
		middleTime - startTime, clock() - middleTime);
}

int main()
{
	const int MaxTestSize = 3000000;
	
	vector<bool> toAlloc(MaxTestSize);
	vector<int> allocSize(MaxTestSize);
	
	fillRequestData(MaxTestSize, toAlloc, allocSize, 1, 1000);
	makeHHeap();
	
	for (int testSizeCur = 100000; testSizeCur < MaxTestSize; testSizeCur *= 2) {
		testRandAllocFree(testSizeCur, toAlloc, allocSize);
	}
	
	for (int testSizeCur = 50000; testSizeCur <= MaxTestSize; testSizeCur += 500000) {
		testSequentialAlloc(testSizeCur, 120, true);
	}

	for (int testSizeCur = 500000; testSizeCur <= MaxTestSize; testSizeCur += 500000) {
		testSequentialAlloc(testSizeCur, 4, true);
	}

	for (int testSizeCur = 500000; testSizeCur <= MaxTestSize; testSizeCur += 500000) {
		testSequentialAlloc(testSizeCur, 120, true);
	}

	testSequentialAlloc(100000, 4096, true);
	testSequentialAlloc(100000, 4096, false);

	HeapDestroy(hHeap);

	return 0;
}