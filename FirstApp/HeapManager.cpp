#include "HeapManager.h"
#include <iostream>
#include <assert.h>
#include <functional>
#include <string> 

 void defaultLimitInitializer(std::vector<DWORD> & limits, DWORD size) {
	limits.push_back(size);
	limits.push_back(size * 32);
}

bool cmp(const CBlockNode* a, const CBlockNode* b) {
		 return a->size < b->size || (a->size == b->size && a->ptr < b->ptr);
	 }

CHeapManager::CHeapManager(size_t minSize_, size_t maxSize_, 
	std::function<void(std::vector<DWORD> &, DWORD)> limitInitializer)
{
	assert(minSize_ <= maxSize_);

	SYSTEM_INFO sSysInfo;
	GetSystemInfo(&sSysInfo);
	pageSize = sSysInfo.dwPageSize;

	minSize = roundUp(minSize_, pageSize);
	maxSize = roundUp(maxSize_, pageSize);

	limitInitializer(sectionLimits, pageSize);
	sectionNumber = sectionLimits.size() + 1;

	allocated = VirtualAlloc(NULL, maxSize, MEM_RESERVE, PAGE_NOACCESS);
	assert(allocated); // change to error handling
	if (minSize_ != 0) {
		LPVOID commited = VirtualAlloc(allocated, minSize, MEM_COMMIT, PAGE_READWRITE);
		assert(commited);
	}
	for (size_t i = 0; i < sectionNumber; ++i) {
		sections.emplace_back(std::set<CBlockNode*, decltype(&cmp)>(&cmp));
	}
	blockMapping.emplace(allocated, CBlockNode(static_cast<char*>(allocated), nullptr, maxSize, true));
	int index = findSection(maxSize);
	sections[index].insert(&blockMapping[allocated]);
}


CHeapManager::~CHeapManager()
{
	bool result = VirtualFree(allocated, 0, MEM_RELEASE);
	assert(result);
}

#ifndef NDEBUG
#define Alloc(x) Alloc(x)
#endif
void * CHeapManager::Alloc(int size)
{
	return Alloc(size, "", -1);
}

#ifndef NDEBUG
#define Alloc(x, y, z) Alloc(x, y, z)
#endif
void * CHeapManager::Alloc(int size, std::string file, int line)
{
	size = roundUp(size, 4);
	size += 2 * canarySize;
	assert(size <= maxSize && size > 0);
	int index = findSection(size);
	while (index < sectionNumber) {
		CBlockNode dummy(0, 0, size, true);
		auto blockIter = sections[index].lower_bound(&dummy);

		if (blockIter == sections[index].end()) {
			index++;
			continue;
		}
		CBlockNode* nodePtr = *blockIter;
		CBlockNode node = *nodePtr;

		assert(node.size >= size);
		sections[index].erase(blockIter);
		nodePtr->free = false;
		nodePtr->file = file;
		nodePtr->line = line;

		if (node.size != size) {
			CBlockNode newNode(node.ptr + size, node.ptr, node.size - size, true);
			nodePtr->size = size;
			blockMapping.emplace(newNode.ptr, newNode);
			auto next = blockMapping.find(newNode.ptr + newNode.size);
			if (next != blockMapping.end()) {
				next->second.prev_ptr = newNode.ptr;
			}
			insertIntoSection(findNode(newNode.ptr));
		}

		LPVOID commited = VirtualAlloc(node.ptr, size, MEM_COMMIT, PAGE_READWRITE);
		assert(commited);
		uint32_t* place = reinterpret_cast<uint32_t*>(node.ptr);
		*place = canary;
		place = reinterpret_cast<uint32_t*>(node.ptr + size - canarySize);
		*place = canary;
		return node.ptr + canarySize;
	}
	assert(false);
}

void CHeapManager::Free(void * ptr)
{
	ptr = static_cast<char*>(ptr) - canarySize;
	CBlockNode * nodePtr = findNode(ptr);
	assert(ptr != nullptr);
	std::string error_message = "Memory acces violation:\nFile: " + 
		nodePtr->file + "\nLine: " + 
		std::to_string(nodePtr->line);
	if (*reinterpret_cast<uint32_t*>(nodePtr->ptr) != canary) {
		throw std::out_of_range(error_message);
	}
	if (*reinterpret_cast<uint32_t*>(nodePtr->ptr + nodePtr->size - canarySize) != canary) {
		throw std::out_of_range(error_message);

	}
	nodePtr->free = true;
	CBlockNode * prev = findNode(nodePtr->prev_ptr);
	CBlockNode * next = findNode(nodePtr->ptr + nodePtr->size);
	bool merged = false;
	if (merged = mergeBlocks(prev, nodePtr)) {
		nodePtr = prev;
	}
	bool rightMerged = mergeBlocks(nodePtr, next);
	merged = merged || rightMerged;
	if (!merged) {
		sections[findSection(nodePtr->size)].insert(nodePtr);
	}
	decommitPages(nodePtr);
}

void CHeapManager::GetBlockInfo(std::vector<std::pair<size_t, bool>>& info)
{
	info.clear();
	for (auto it = blockMapping.begin(); it != blockMapping.end(); ++it) {
		info.push_back({ static_cast<char*>(it->first) - allocated, it->second.free });
	}
}

size_t CHeapManager::roundUp(size_t num, size_t div)
{
	return div * ((num + div - 1) / div);
}

size_t CHeapManager::findSection(size_t size)
{
	int index = -1;
	while (++index < sectionLimits.size() && size > sectionLimits[index]);
	return index;
}

void CHeapManager::insertIntoSection(CBlockNode * it)
{
	size_t index = findSection(it->size);
	sections[index].insert(it);
}

bool CHeapManager::mergeBlocks(CBlockNode * left, CBlockNode * right)
{
	if (left == nullptr || right == nullptr || !left->free || !right->free) {
		return false;
	}
	assert(left->ptr + left->size == right->ptr);

	size_t index = findSection(left->size);
	sections[index].erase(left);
	index = findSection(right->size);
	sections[index].erase(right);

	left->size += right->size;
	auto next = blockMapping.find(left->ptr + left->size);
	if (next != blockMapping.end()) {
		next->second.prev_ptr = left->ptr;
	}
	blockMapping.erase(right->ptr);

	index = findSection(left->size);
	sections[index].insert(left);
	return true;
}

CBlockNode * CHeapManager::findNode(void * ptr)
{
	auto it = blockMapping.find(ptr);
	if (it == blockMapping.end()) {
		return nullptr;
	}
	return &(it->second);
}

void CHeapManager::decommitPages(CBlockNode * node)
{
	size_t diff = roundUp(node->ptr - allocated, pageSize);
	char* begin = static_cast<char*>(allocated) + diff;
	size_t upDiff = (node->ptr + node->size - allocated) / pageSize * pageSize;
	char* end = static_cast<char*>(allocated) + upDiff;
	if (end > begin) {
		bool result = VirtualFree(begin, upDiff - diff, MEM_DECOMMIT);
		assert(result);
	}
}

int main_() {
	std::cout << "hello";
	return 0;
}
