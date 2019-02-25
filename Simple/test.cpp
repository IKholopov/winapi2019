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
	void* ptr = allocator.Alloc(4);
	char* mem = static_cast<char*>(ptr);

	ASSERT_EQ(mem[0], 0);
	mem[4] = 'a';

	std::vector<std::pair<size_t, bool>> info;
	std::vector<std::pair<size_t, bool>> expectedInfo;
	expectedInfo.push_back({ 0, false });
	expectedInfo.push_back({ 4 + 2 * CHeapManager::canarySize, true });
	allocator.GetBlockInfo(info);
	ASSERT_EQ(info, expectedInfo);
}

TEST(Simple, OutOfMemory) {
	auto allocator = CHeapManager(0, 5 * PageSize);
	void* ptr = allocator.Alloc(4); // 39 - allocation place
	char* mem = static_cast<char*>(ptr);

	ASSERT_EQ(mem[0], 0);
	mem[4] = 'a';

	try {
		allocator.Free(ptr);
		FAIL("Exception expected");
	}
	catch (std::out_of_range& exception) {
		std::string what = exception.what();
		std::string end = what.substr(what.length() - 2);
		// Just check line of code
		ASSERT_EQ(end, "39");
	}
}

TEST(Simple, MergeBlocks) {
	auto allocator = CHeapManager(0, PageSize);

	std::random_device rd;  
	std::mt19937 gen(rd()); 
	std::uniform_int_distribution<> dis(1, PageSize / 16);

	size_t size1 = dis(gen) * 8;
	size_t size2 = dis(gen) * 8;
	void* ptr = allocator.Alloc(size1);
	void* midPtr = allocator.Alloc(size2);

	std::vector<std::pair<size_t, bool>> info;
	std::vector<std::pair<size_t, bool>> expectedInfo;
	expectedInfo.push_back({ 0, false });
	expectedInfo.push_back({ size1 + 2 * CHeapManager::canarySize, false });
	expectedInfo.push_back({ size1 + 4 * CHeapManager::canarySize + size2, true });

	allocator.GetBlockInfo(info);
	std::sort(info.begin(), info.end());
	ASSERT_EQ(info, expectedInfo);

	expectedInfo.clear();
	allocator.Free(ptr);
	allocator.Free(midPtr);
	expectedInfo.push_back({ 0, PageSize });
	allocator.GetBlockInfo(info);
	ASSERT_EQ(expectedInfo, info);
}

TEST(Simple, StressRandom) {
	size_t size = 10000 * PageSize;
	auto allocator = CHeapManager(0, size);
	std::vector<void*> ptrs;
	size_t iterations = 1000;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, size / iterations / 8);
	std::uniform_int_distribution<> unfairCoin(0, 2);

	for (size_t i = 0; i < iterations; ++i) {
		size_t size = dis(gen) * 8;
		void* ptr = allocator.Alloc(size);
		ASSERT_NE(ptr, nullptr);
		ptrs.push_back(ptr);
		if (ptrs.size() > 0 && unfairCoin(gen) == 0) {
			size_t index = std::uniform_int_distribution<>(0, ptrs.size() - 1)(gen);
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
	const size_t iterations = 1000;
	CCustomClass* instanses[iterations];
	clock_t begin = std::clock();
	for (size_t i = 0; i < iterations; ++i) {
		instanses[i] = new CCustomClass;
	}
	clock_t end = clock();
	double elapsedTime = double(end - begin);
	for (size_t i = 0; i < iterations; ++i) {
		delete instanses[i];
	}

	begin = std::clock();
	for (size_t i = 0; i < iterations; ++i) {
		instanses[i] = new CCustomClass;
	}
	end = clock();
	double elapsedStandartTime = double(end - begin);
	for (size_t i = 0; i < iterations; ++i) {
		delete instanses[i];
	}
	testing::internal::CaptureStdout();
	std::cout << "Custom allocator: " << elapsedTime << std::endl;
	std::cout << "Standart allocator: " << elapsedStandartTime << std::endl;
	std::string output = testing::internal::GetCapturedStdout();
	ASSERT_LE(elapsedTime, elapsedStandartTime * 2);
	ASSERT_GE(elapsedTime, elapsedStandartTime);
}




int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}