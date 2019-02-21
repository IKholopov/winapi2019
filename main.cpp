#include<iostream>
#include"HeapManager.h"


int main() {
	HeapManager HM(100, 1000);
	HM.debug_print();
	char* x = (char*) HM.Alloc(500);
	HM.debug_print();
	char* y = (char*) HM.Alloc(15);
	HM.debug_print();
	
	HM.Free(x);
	HM.debug_print();
	HM.Free(y);
	HM.debug_print();
	int i;
	std::cin >> i;
	return 0;
}