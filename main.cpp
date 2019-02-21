#include<iostream>
#include"HeapManager.h"


int main() {
	HeapManager HM(100, 1000);
	HM._debug_print();
	char* x = (char*) HM.Alloc(985);
	HM._debug_print();
	char* y = (char*) HM.Alloc(15);
	HM._debug_print();
	
	HM.Free(x);
	HM._debug_print();
	HM.Free(y);
	HM._debug_print();
	int i;
	std::cin >> i;
	return 0;
}