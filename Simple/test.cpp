#include <random>
#include <algorithm>
#include <chrono>
#include <ctime>

#include "pch.h"
#include "../FirstApp/HeapManager.h"
#include "../FirstApp/CustomClass.h"

static const size_t PageSize = 4096;
CHeapManager globalAllocator(0, ArenaSize);

void* operator new(std::size_t sz, bool b) 
{
	return globalAllocator.Alloc(sz);
}
void operator delete(void* ptr, bool b)
{
	globalAllocator.Free(ptr);
}

TEST(Simple, OneBlock) {
	auto allocator = CHeapManager(0, 5 * PageSize);
	void* ptr = allocator.Alloc(1);
	char* mem = static_cast<char*>(ptr);

	ASSERT_EQ(mem[0], 0);
	ASSERT_EQ(mem[PageSize - 1], 0);

	std::vector<std::pair<size_t, bool>> info;
	std::vector<std::pair<size_t, bool>> expectedInfo;
	expectedInfo.push_back({ 0, false });
	expectedInfo.push_back({ 4, true });
	allocator.GetBlockInfo(info);
	ASSERT_EQ(info, expectedInfo);
}

TEST(Simple, MergeBlocks) {
	auto allocator = CHeapManager(0, PageSize);
	size_t size1 = (std::rand() % (PageSize / 8)) * 8;
	size_t size2 = (std::rand() % (PageSize / 8)) * 8;
	void* ptr = allocator.Alloc(size1);
	void* midPtr = allocator.Alloc(size2);

	std::vector<std::pair<size_t, bool>> info;
	std::vector<std::pair<size_t, bool>> expectedInfo;
	expectedInfo.push_back({ 0, false });
	expectedInfo.push_back({ size1, false });
	expectedInfo.push_back({ size1 + size2, true });

	allocator.GetBlockInfo(info);
	ASSERT_EQ(info, expectedInfo);

	expectedInfo.clear();
	allocator.Free(ptr);
	allocator.Free(midPtr);
	expectedInfo.push_back({ 0, PageSize });
	allocator.GetBlockInfo(info);
	ASSERT_EQ(expectedInfo, info);
}

TEST(Simple, StressRandom) {
	auto allocator = CHeapManager(0, 10000 * PageSize);
	std::vector<void*> ptrs;
	size_t iterations = 1000;
	for (size_t i = 0; i < iterations; ++i) {
		size_t size = (std::rand() % (PageSize / 64) + 1) * 8;
		void* ptr = allocator.Alloc(size);
		ASSERT_NE(ptr, nullptr);
		ptrs.push_back(ptr);
		if (std::rand() % 3 == 0) {
			size_t index = std::rand() % ptrs.size();
			allocator.Free(ptrs[index]);
			ptrs.erase(ptrs.begin() + index);
		}
	}
	std::random_shuffle(ptrs.begin(), ptrs.end());
	for (auto ptr : ptrs) {
		allocator.Free(ptr);
	}
	std::vector<std::pair<size_t, bool>> info;
	std::vector<std::pair<size_t, bool>> expectedInfo;
	expectedInfo.push_back({ 0, PageSize });

	allocator.GetBlockInfo(info);
	ASSERT_EQ(info, expectedInfo);
}

TEST(Simple, OverloadedNew) {
	const size_t iterations = 10000;
	CCustomClass* instanses[iterations];
	clock_t begin = std::clock();
	for (size_t i = 0; i < iterations; ++i) {
		instanses[i] = new CCustomClass;
	}
	clock_t end = clock();
	double elapsed_secs = double(end - begin);
	for (size_t i = 0; i < iterations; ++i) {
		delete instanses[i];
	}

	begin = std::clock();
	for (size_t i = 0; i < iterations; ++i) {
		instanses[i] = new CCustomClass;
	}
	end = clock();
	double elapsed_secs1 = double(end - begin);
	for (size_t i = 0; i < iterations; ++i) {
		delete instanses[i];
	}
	ASSERT_LE(elapsed_secs, elapsed_secs1 * 10);
}

TEST(Simple, PlacementNew) {
	size_t num = 1000;
	size_t iterations = 6000;
	int* buf;
	size_t before = GetTickCount();
	for (size_t i = 0; i < iterations; ++i) {
		buf = new (true) int(1);
	}
	size_t after = GetTickCount();
	size_t time = after - before;

	int* buf1;
	before = GetTickCount();
	for (size_t i = 0; i < iterations; ++i) {
		buf1 = new int(1);
	}
	after = GetTickCount();
	size_t standartTime = after - before;
	delete buf1;

	ASSERT_LE(time, standartTime * 10);
}



int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}