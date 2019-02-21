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
	// max_size - размер зарезервированного региона пам€ти при создании
	// min_size - размер закоммиченного региона при создании
	HeapManager(size_t min_size, size_t max_size);
	// ¬с€ пам€ть должна быть освобождена после окончании работы
	~HeapManager();
	// јллоцировать регион размера size
	void* Alloc(size_t size);
	// ќсвободить пам€ть
	void Free(void* ptr);
	void _debug_print() {
		std::cout << "> ";
		for (auto i = allblocks.begin(); i != allblocks.end(); ++i) {
			if ((*i)->free) std::cout << '*';
			std::cout << (*i)->size << ' ';
		}
		std::cout << std::endl;
	}
private:
	struct block {
		block(void* ptr, size_t size, bool free);
		void* ptr;
		bool free;
		size_t size;
		std::list<block*>::iterator inlist_iter;
		std::multimap<size_t, block*>::iterator inmap_iter;
	};
	void* start;
	size_t reserved_size, commited_size, min_commited;
	std::list<block*> allblocks;
	std::multimap<size_t, block*> freeblocks;
	std::unordered_map<void*, block*> occupiedblocks;
};