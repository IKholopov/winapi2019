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
	// max_size - ðàçìåð çàðåçåðâèðîâàííîãî ðåãèîíà ïàìÿòè ïðè ñîçäàíèè
	// min_size - ðàçìåð çàêîììè÷åííîãî ðåãèîíà ïðè ñîçäàíèè
	HeapManager(size_t min_size, size_t max_size);
	// Âñÿ ïàìÿòü äîëæíà áûòü îñâîáîæäåíà ïîñëå îêîí÷àíèè ðàáîòû
	~HeapManager();
	// Àëëîöèðîâàòü ðåãèîí ðàçìåðà size
	void* Alloc(size_t size);
	// Îñâîáîäèòü ïàìÿòü
	void Free(void* ptr);
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
