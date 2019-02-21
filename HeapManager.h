#pragma once
#include <windows.h>
#include<memoryapi.h>
#include<list>
#include<map>
#include<unordered_map>

#include<iostream>

class HeapManager {
public:
	// max_size - размер зарезервированного региона пам€ти при создании
	// min_size - размер закоммиченного региона при создании
	HeapManager(size_t min_size, size_t max_size) : min_reserved(min_size), reserved_size(max_size), commited_size(min_size) {
		start = VirtualAlloc(NULL, reserved_size, MEM_RESERVE, PAGE_READWRITE);
		if (start == 0)
			throw 111;
		if (VirtualAlloc(start, commited_size, MEM_COMMIT, PAGE_READWRITE) == 0)
			throw 222;
		block* newblock = new block(start, commited_size, 1);

		newblock->inmap_iter = freeblocks.insert(std::make_pair(commited_size, newblock));
		newblock->inlist_iter = allblocks.insert(allblocks.end(), newblock);
		//allblocks_list.erase(newblock->inlist_iter);
		//std::cout << 100001 << std::endl;
	}
	// ¬с€ пам€ть должна быть освобождена после окончании работы
	~HeapManager() {
		//VirtualFree
	}
	// јллоцировать регион размера size
	void* Alloc(size_t size) {
		//find min in maper
		auto iter_found = freeblocks.lower_bound(size);
		if (iter_found == freeblocks.end()) {
			size_t required = size - allblocks.back()->size * allblocks.back()->free;
			if (reserved_size - commited_size < required)
				throw 333;
			if (VirtualAlloc(static_cast<char*>(start) + commited_size, required, MEM_COMMIT, PAGE_READWRITE) == 0)
				throw 444;
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
			return newblock->ptr;
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
	// ќсвободить пам€ть
	void Free(void* ptr) {
		block* block_to_free = occupiedblocks[ptr];
		if (block_to_free == 0)
			throw 555;
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
		else if (reserved_size > min_reserved) {
			need_delete = true;
			if (reserved_size - newblock_size >= min_reserved) {
				if (VirtualFree(newblock_ptr, newblock_size, MEM_DECOMMIT) == 0)
					throw 666;
				need_create = false;
			}
			else {
				if (VirtualFree(static_cast<char*>(newblock_ptr) + (min_reserved + newblock_size - reserved_size), reserved_size - min_reserved, MEM_DECOMMIT) == 0)
					throw 777;
				newblock_size = min_reserved + newblock_size - reserved_size;
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
	void debug_print() {
		for (auto i = allblocks.begin(); i != allblocks.end(); ++i) {
			if ((*i)->free) std::cout << '*';
			std::cout << (*i)->size << ' ';
		}
		std::cout << std::endl;
	}
private:
	struct block {
		block(void* ptr, size_t size, bool free) : ptr(ptr), size(size), free(free) {}
		void* ptr;
		bool free;
		size_t size;
		std::list<block*>::iterator inlist_iter;
		std::multimap<size_t, block*>::iterator inmap_iter;
	};
	void* start;
	size_t reserved_size, commited_size, min_reserved;
	std::list<block*> allblocks;
	std::multimap<size_t, block*> freeblocks;
	std::unordered_map<void*, block*> occupiedblocks;
};