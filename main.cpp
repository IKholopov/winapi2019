#include<iostream>
#include"HeapManager.h"
#include"Tester.h"

HeapManager* HM;

void* new_alloc(size_t s) {
	return static_cast<void*>(new char[s]);
}

void new_free(void* ptr) {
	delete[] ptr;
}

void* hm_alloc(size_t s) {
	return HM->Alloc(s);
}

void hm_free(void* ptr) {
	return HM->Free(ptr);
}

int main() {
	
	Tester test;
	int allocations_num = 100;
	int sum_allocation_size = 200; //MBytes
	std::cout << "allocatios num: " << allocations_num << std::endl;
	std::cout << "sum allocation size: " << sum_allocation_size << "MB" << std::endl;

	//Генерирует список команд из alloc/free (не исполняя). Длинна списка = 2*allocations_num
	//Суммарное кол-во памяти, которое запросят команды из списка - sum_allocation_size. 
	//Возврощает сколько памяти максимально в один момент будет запрошено во время теста
	size_t max_allocated = test.NewRandomCommands(sum_allocation_size * 1024 * 1024, allocations_num); 
	
	//Раскомментировать для вывода списока сгенерированных команд 
	//std::cout << std::endl << "Random testing sheme: " << std::endl;
	//test.PrintCommands(); 
	
	//Добавляем сравниваемые аллокаторы
	test.AddAllocator(&new_alloc, &new_free, "new");
	HM = new HeapManager(max_allocated, 10 * max_allocated);
	test.AddAllocator(&hm_alloc, &hm_free, "HeapManager");

	std::cout << std::endl << "First competition" << std::endl;
	std::cout << "(max allocated at moment: " << max_allocated / 1024 / 1024 << "MB)" << std::endl;
	test.Start();

	std::cout << std::endl << "Increase allocations num to 100000" << std::endl;
	allocations_num = 100000;
	max_allocated = test.NewRandomCommands(sum_allocation_size * 1024 * 1024, allocations_num);

	std::cout << std::endl << "Second competition" << std::endl;
	std::cout << "(max allocated at moment: " << max_allocated / 1024 / 1024 << "MB)" << std::endl;
	test.Start();
	
	int z; std::cin >> z;

	return 0;
}