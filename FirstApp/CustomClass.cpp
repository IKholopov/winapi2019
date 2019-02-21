#include "CustomClass.h"



CCustomClass::CCustomClass()
{

}


CCustomClass::~CCustomClass()
{
}

void * CCustomClass::operator new(size_t sz)
{
	return customAllocator.Alloc(sz);
}

void CCustomClass::operator delete(void * m)
{
	customAllocator.Free(m);
}
