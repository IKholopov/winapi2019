// ConsoleApplication2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.

#include "pch.h"

#include <iostream>
#include <random>
#include <ctime>

#include "windows.h"
#include "CAllocator.h"

using std::cout;
using std::endl;

const int AllocatorCapacity = 1024 * 1024 * 500;
CAllocator alloc(4, AllocatorCapacity); // 500 mb
HANDLE hHeap;

void fillRequestData(int TestSize, bool** toAlloc, int** allocSize, int min, int max, int chance) {
	std::random_device rd1, rd2, rd3;
	std::mt19937 gen1(rd1()), gen2(rd2()), gen3(rd3());
	std::uniform_int_distribution<> dis1(1, 2), dis2(min, max), dis3(0, chance);

	*toAlloc = new bool[TestSize];
	*allocSize = new int[TestSize];

	for (int i = 0; i < TestSize; ++i) {
		(*toAlloc)[i] = (dis1(rd1) % 2 == 0);
		(*allocSize)[i] = dis2(rd2);
		if (chance >= 0 && dis3(rd3) == 0) {
			(*allocSize)[i] = 1024 * 200; // 200kb
		}
	}
}

void makeHHeap() {
	hHeap = HeapCreate(0, 0, AllocatorCapacity);
}

void testRandAllocFree(int TestSize, bool* toAlloc, int* allocSize) {
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
	bool* toAlloc = nullptr;
	int* allocSize = nullptr;
	
	fillRequestData(MaxTestSize, &toAlloc, &allocSize, 1, 1000, -1);
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
	delete toAlloc;
	delete allocSize;

	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
