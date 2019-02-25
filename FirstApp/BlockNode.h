#pragma once
#include <string>
struct CBlockNode
{
	CBlockNode(char* ptr_, char* prev_ptr_, size_t size_, bool free_, const std::string& file_, int line_) : 
		ptr(ptr_), 
		prev_ptr(prev_ptr_), 
		size(size_), 
		free(free_),
		file(file_),
		line(line_)
		{}
	CBlockNode(char* ptr_, char* prev_ptr_, size_t size_, bool free_) : 
		CBlockNode(ptr_, prev_ptr_, size_, free_, "", -1)
		{}
	CBlockNode();
	~CBlockNode();

	char * ptr;
	size_t size;
	char * prev_ptr;
	bool free;
	std::string file;
	int line;
};

