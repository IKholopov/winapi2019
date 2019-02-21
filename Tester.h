#pragma once

#include <random>
#include <vector>
#include <iostream>
#include <string>
#include <assert.h>
#include <time.h>

class Tester {
public:
	void AddAllocator(void*(*alloc)(size_t), void(*free)(void*), std::string name);

	int AddRandomCommands(size_t sum_size, size_t allocs_num);

	int NewRandomCommands(size_t sum_size, size_t allocs_num);

	void PrintCommands();

	void Start();


private:
	struct command {
		command(size_t size);
		size_t size = 0;
		size_t ptr_index;
		bool type = 1; //alloc = 1//free = 0//
	};
	std::vector<void*> ptrs;
	struct unit{
		unit(void*(*alloc)(size_t), void(*free)(void*), std::string name, Tester* base);
		void operator() (command& c);
		Tester* base;
		void*(*alloc)(size_t);
		void(*free)(void*);
		std::string name;
	};
	std::vector<unit> units;
	std::vector<command> commands;
};