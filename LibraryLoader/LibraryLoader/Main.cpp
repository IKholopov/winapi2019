#include <windows.h>
#include <ImageHlp.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <string>

#include "LibraryLoader.h"


int main() {
	CLibraryLoader loader("../DynamicFib.dll");
	auto exports = loader.ListExports();

	for (auto&& s : exports) {
		std::wcout << s << std::endl;
	}

	FunctionReturnType fib = loader.LoadFunction(L"FibRecursive");
	std::cout << fib(5);

	return 0;
}