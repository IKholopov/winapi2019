#pragma once

#include <Windows.h>
#include <set>
#include <map>
#include <vector>


//—труктура дл€ хранени€ свободных отрезков

struct CMemorySegment {
	char* prev{0};
	char* dataPointer{0};
	int size{0};
	bool isBusy{false};

	CMemorySegment() = default;
	CMemorySegment(char* prev,
		char* dataPointer,
		int size,
		bool isBusy);
};


//оператор сравнени€ дл€ поиска по длиине 
bool operator<( const CMemorySegment& first, const CMemorySegment& second );



//-------------------------------------------------------------------------------------------------


//компоратор дл€ поиска по указателью

class CMemoryPointerCompartor {
public:
	bool operator()(const CMemorySegment& first, const CMemorySegment& second) const;
};


//-------------------------------------------------------------------------------------------------


//јллокатор

class HeapManager {
public:
	// max_size - размер зарезервированного региона пам€ти при создании
	// min_size - размер закоммиченного региона при создании
	HeapManager( int min_size, int max_size );
	// ¬с€ пам€ть должна быть освобождена после окончании работы
	~HeapManager();
	
	// јллоцировать регион размера size
	void* Alloc( int size );
	// ќсвободить пам€ть
	void Free( void* ptr );

private:
	int pageSize{4096};
	int maxSize{4096};
	int minSize{1024};
	char* dataPointer{0};
	std::set<CMemorySegment> freeSegments;
	std::map<char* , CMemorySegment> allSegments;
	std::set<char*> busyPointers;

	std::vector<char> isCommited;

	int roundValue( int value ) const;
	char* tryGetPointer( int size );
	void decommitPages( const CMemorySegment& memorySegment );
};
