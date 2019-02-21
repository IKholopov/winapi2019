#include "Tester.h"


void Tester::AddAllocator(void*(*alloc)(size_t), void(*free)(void *), std::string name)
{
	units.push_back(unit(alloc, free, name, this));
}

int Tester::AddRandomCommands(size_t sum_size, size_t allocs_num) {
	assert(sum_size >= allocs_num);

	std::random_device rd;
	std::mt19937 gen(rd());

	size_t remaining_size = sum_size - 1;
	std::vector<command> shuffled;
	for (size_t i = 0; i < allocs_num - 1; i++) {
		std::uniform_int_distribution<> dis(1, 2 * remaining_size / (allocs_num - i));
		int size = dis(gen);
		remaining_size -= size;
		shuffled.push_back(size);
	}
	shuffled.push_back(remaining_size + 1);

	int ptr_index = 0;
	int max_allocated = 0;
	int current_allocated = 0;
	while (shuffled.size() > 0) {
		std::uniform_int_distribution<> dis(0, shuffled.size() - 1);
		int i = dis(gen);
		commands.push_back(shuffled[i]);
		if (shuffled[i].type == 1) {
			current_allocated += shuffled[i].size;
			if (current_allocated > max_allocated)
				max_allocated = current_allocated;

			shuffled[i].ptr_index = ptr_index;
			ptr_index++;
			shuffled[i].type = 0;
		}
		else {
			current_allocated -= shuffled[i].size;
			shuffled.erase(shuffled.begin() + i);
		}
	}

	return max_allocated;
}

int Tester::NewRandomCommands(size_t sum_size, size_t allocs_num)
{
	commands.clear();
	return AddRandomCommands(sum_size, allocs_num);
}

void Tester::PrintCommands() {
	if (commands.size() == 0) {
		std::cout << "Generate commands first" << std::endl;
		return;
	}
	int ptr_index = 0;
	for (auto& c : commands)
		if (c.type == 1)
			std::cout << "allocate" << '(' << c.size << ") -> " << ptr_index++ << std::endl;
		else
			std::cout << "free" << '(' << c.ptr_index << ')' << std::endl;
}

void Tester::Start() {
	for (auto& u : units) {
		ptrs.clear();
		int start_time = clock();
		for (auto& c : commands) u(c);
		std::cout << u.name << ": " << clock() - start_time << " ms" << std::endl;
	}
}

Tester::unit::unit(void*(*alloc)(size_t), void(*free)(void *), std::string name, Tester* base) : alloc(alloc), free(free), name(name), base(base) {}

void Tester::unit::operator()(command & c)
{
	if (c.type == 0)
		(*free)(base->ptrs[c.ptr_index]);
	else 
		base->ptrs.push_back((*alloc)(c.size));
}

Tester::command::command(size_t size) : size(size) {}
