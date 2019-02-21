#pragma once
struct CBlockNode
{
	CBlockNode(char* ptr_, char* prev_ptr_, size_t size_, bool free_) : 
		ptr(ptr_), 
		prev_ptr(prev_ptr_), 
		size(size_), 
		free(free_) {}
	CBlockNode();
	~CBlockNode();

	char * ptr;
	size_t size;
	char * prev_ptr;
	bool free;
};

