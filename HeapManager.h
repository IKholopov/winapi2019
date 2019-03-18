#pragma once

#include <windows.h>
#include <memoryapi.h>
#include <list>
#include <map>
#include <unordered_map>
#include <assert.h>

#include<iostream>

class HeapManager {
public:
	HeapManager(size_t min_size, size_t max_size);
	~HeapManager();
	void* Alloc(size_t size);
	void Free(void* ptr);

private:
	struct Block {
		Block(void* ptr, size_t size, bool free);
		void* ptr;
		bool free;
		size_t size;
		std::list<Block*>::iterator inlist_iter;
		std::multimap<size_t, Block*>::iterator inmap_iter;
	};
	void* start;
	size_t reserved_size, commited_size, min_commited;
	std::list<Block*> allblocks;
	std::multimap<size_t, Block*> freeblocks;
	std::unordered_map<void*, Block*> occupiedblocks;
};