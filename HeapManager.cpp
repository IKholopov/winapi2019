#pragma once

#include "HeapManager.h"

HeapManager::HeapManager(size_t min_size, size_t max_size) : min_commited(min_size), reserved_size(max_size), commited_size(min_size) {
	assert(max_size > min_size);
	start = VirtualAlloc(NULL, reserved_size, MEM_RESERVE, PAGE_READWRITE);
	assert(start != 0);
	assert(VirtualAlloc(start, commited_size, MEM_COMMIT, PAGE_READWRITE) != 0);

	block* newblock = new block(start, commited_size, 1);
	newblock->inmap_iter = freeblocks.insert(std::make_pair(commited_size, newblock));
	newblock->inlist_iter = allblocks.insert(allblocks.end(), newblock);
}

HeapManager::~HeapManager() {
	for (auto &i : allblocks) //.begin(); i != allblocks.end(); i = allblocks.erase(i))
		delete i;
	VirtualFree(start, 0, MEM_RELEASE);
}

void * HeapManager::Alloc(size_t size) {
	//find min in maper
	auto iter_found = freeblocks.lower_bound(size);
	if (iter_found == freeblocks.end()) {
		size_t required = size - allblocks.back()->size * allblocks.back()->free;
		assert(reserved_size >= required + commited_size);
		assert(VirtualAlloc(static_cast<char*>(start) + commited_size, required, MEM_COMMIT, PAGE_READWRITE) != 0);
		commited_size += required;

		void* newblock_ptr;

		if (allblocks.back()->free) {
			newblock_ptr = allblocks.back()->ptr;
			block* last_free = allblocks.back();
			freeblocks.erase(last_free->inmap_iter);
			allblocks.erase(last_free->inlist_iter);
			delete last_free;
		}
		else {
			newblock_ptr = static_cast<char*>(allblocks.back()->ptr) + allblocks.back()->size;
		}

		block* newblock = new block(newblock_ptr, size, 0);
		//newblock->inmap_iter = freeblocks.insert(std::make_pair(size, newblock));
		newblock->inlist_iter = allblocks.insert(allblocks.end(), newblock);
		occupiedblocks[newblock_ptr] = newblock;
		return newblock_ptr;
	}

	block* current_block = iter_found->second;

	freeblocks.erase(iter_found);
	auto iter = allblocks.erase(current_block->inlist_iter);

	block* occupied_block = new block(current_block->ptr, size, 0);
	occupied_block->inlist_iter = allblocks.insert(iter, occupied_block);

	if (current_block->size != size) {
		block* free_block = new block(static_cast<char*>(current_block->ptr) + size, current_block->size - size, 1);
		free_block->inmap_iter = freeblocks.insert(std::make_pair(current_block->size - size, free_block));
		free_block->inlist_iter = allblocks.insert(iter, free_block);
	}

	delete current_block;

	occupiedblocks[occupied_block->ptr] = occupied_block;
	return occupied_block->ptr;
}

void HeapManager::Free(void * ptr) {
	block* block_to_free = occupiedblocks[ptr];
	assert(block_to_free != 0);
	occupiedblocks.erase(ptr);

	void* newblock_ptr = block_to_free->ptr;
	size_t newblock_size = block_to_free->size;
	bool need_delete = false;
	bool need_create = true;

	auto iter = block_to_free->inlist_iter;

	if (iter != allblocks.begin()) {
		block* left = *(--iter);
		if (left->free) {
			newblock_ptr = left->ptr;
			newblock_size += left->size;
			freeblocks.erase(left->inmap_iter);
			allblocks.erase(left->inlist_iter);
			need_delete = true;
		}
	}

	iter = block_to_free->inlist_iter;
	if (++iter != allblocks.end()) {
		block* right = *iter;
		if (right->free) {
			newblock_size += right->size;
			freeblocks.erase(right->inmap_iter);
			allblocks.erase(right->inlist_iter);
			need_delete = true;
		}
	}
	else if (commited_size > min_commited) {
		need_delete = true;
		if (commited_size - newblock_size >= min_commited) {
			assert(VirtualFree(newblock_ptr, newblock_size, MEM_DECOMMIT) != 0);
			need_create = false;
		}
		else {
			assert(VirtualFree(static_cast<char*>(newblock_ptr) + (min_commited + newblock_size - commited_size), commited_size - min_commited, MEM_DECOMMIT) != 0);
			newblock_size = min_commited + newblock_size - commited_size;
		}
	}

	if (need_delete) {
		iter = allblocks.erase(block_to_free->inlist_iter);
		if (iter != allblocks.begin()) --iter;
		delete block_to_free;
		if (need_create) {
			block* newblock = new block(newblock_ptr, newblock_size, 1);
			newblock->inmap_iter = freeblocks.insert(std::make_pair(newblock_size, newblock));
			newblock->inlist_iter = allblocks.insert(iter, newblock);
		}
	}
	else {
		block_to_free->free = true;
		block_to_free->inmap_iter = freeblocks.insert(std::make_pair(block_to_free->size, block_to_free));
	}
}

HeapManager::block::block(void * ptr, size_t size, bool free) : ptr(ptr), size(size), free(free) {}
