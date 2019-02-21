#include<iostream>
#include"HeapManager.h"
#include"Tester.h"

HeapManager* HM;

void* new_alloc(size_t s) {
	void* ptr = static_cast<void*>(new char[s]);
	return ptr;
}

void new_free(void* ptr) {
	delete[] ptr;
}

void my_alloc(size_t s) {

}

void* hm_alloc(size_t s) {
	return HM->Alloc(s);
}

void hm_free(void* ptr) {
	return HM->Free(ptr);
}

int main() {
	
	Tester test;
	int test_allocations_num = 30;
	int sum_allocation_size = 100; //MBytes

	size_t max_allocated = test.AddRandomCommands(sum_allocation_size * 1024 * 1024, test_allocations_num);
	std::cout << "Random testing sheme: " << std::endl;
	test.PrintCommands();

	std::cout << "max allocated at moment: " <<  max_allocated / 1024 / 1024 << "MB" << std::endl;
	
	test.AddAllocator(&new_alloc, &new_free, "new");
	HM = new HeapManager(max_allocated, 10 * max_allocated);
	test.AddAllocator(&hm_alloc, &hm_free, "HeapManager");

	std::cout << std::endl << "Start competition" << std::endl;
	test.Start();
	
	int z; std::cin >> z;
	return 0;
}