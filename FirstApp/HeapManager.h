#pragma once

#include <Windows.h>
#include <set>
#include <unordered_map>
#include <vector>
#include <functional>
#include <list>
#include "BlockNode.h"

void defaultLimitInitializer(std::vector<DWORD> & limits, DWORD size);

bool cmp(const CBlockNode* a, const CBlockNode* b);


class CHeapManager {
public:
	// maxSize_ - reserved memory
	// minSize_ - commited memory
	CHeapManager(size_t minSize_, size_t maxSize_, 
		std::function<void(std::vector<DWORD> &, DWORD)> limitInitializer = defaultLimitInitializer);
	~CHeapManager();

	void* Alloc(int size);
	void Free(void* ptr);

	void GetBlockInfo(std::vector<std::pair<size_t, bool>>& info);

private:
	size_t minSize;
	size_t maxSize;
	LPVOID allocated;
	DWORD pageSize;
	size_t sectionNumber;

	std::vector<DWORD> sectionLimits;

	std::vector<std::set<CBlockNode*, decltype(&cmp)>> sections;
	std::unordered_map<void*, CBlockNode> blockMapping;

	size_t roundUp(size_t num, size_t div);

	size_t findSection(size_t size);
	void insertIntoSection(CBlockNode * it);

	bool mergeBlocks(CBlockNode * left, CBlockNode * right);
	CBlockNode * findNode(void* ptr);
	void decommitPages(CBlockNode * node);
	
};
