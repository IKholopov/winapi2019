#include <iostream>

#include "HeapManager.h"

CMemorySegment::CMemorySegment(char* prev,
	char* dataPointer,
	int size,
	bool isBusy = false):
	prev( prev ),
	dataPointer( dataPointer ),
	size( size ),
	isBusy( isBusy )
{
}

bool operator<(const CMemorySegment& first, const CMemorySegment& second) 
{
	return first.size == second.size ? first.dataPointer < second.dataPointer : first.size < second.size;
}


//-------------------------------------------------------------------------------------------------


bool CMemoryPointerCompartor::operator()( const CMemorySegment& first, const CMemorySegment& second ) const
{
	return first.dataPointer < second.dataPointer;
}


//-------------------------------------------------------------------------------------------------

HeapManager::HeapManager(int minSize_, int maxSize_) :
	minSize( roundValue( minSize_ ) ),
	maxSize( roundValue( maxSize_ ) ) 
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	pageSize = systemInfo.dwPageSize;

	dataPointer = static_cast<char*>( VirtualAlloc( NULL, maxSize, MEM_RESERVE, PAGE_READWRITE ) );
	
	VirtualAlloc(NULL, minSize, MEM_COMMIT, PAGE_READWRITE);
	
	freeSegments.insert(CMemorySegment( dataPointer , dataPointer + minSize, maxSize - minSize ) );
	freeSegments.insert( CMemorySegment( nullptr, dataPointer, minSize ) );
	allSegments[dataPointer] = CMemorySegment( nullptr, dataPointer, maxSize );
	allSegments[dataPointer + minSize] = CMemorySegment(nullptr, dataPointer + minSize, maxSize - minSize );
}


int HeapManager::roundValue( int value ) const
{
	return value % 4 == 0 ? value : value - value % 4 + 4;
}

char* HeapManager::tryGetPointer( int size )
{
	char* pointer = nullptr; 
	auto iter = freeSegments.lower_bound( CMemorySegment( 0, 0, size ) );
	if( iter != freeSegments.end() ) {
		pointer = iter->dataPointer;
		int restSize = iter->size - size;
		CMemorySegment segment = *iter;
		freeSegments.erase(iter);
		allSegments[segment.dataPointer] = CMemorySegment(segment.prev, segment.dataPointer, size, true);
		busyPointers.insert(pointer);

		if( restSize > 0 ) {
			allSegments[segment.dataPointer + size] = CMemorySegment(segment.dataPointer, segment.dataPointer + size, restSize );
			freeSegments.insert(CMemorySegment(segment.dataPointer, segment.dataPointer + size, restSize));
		} 
		

		VirtualAlloc(pointer, size, MEM_COMMIT, PAGE_READWRITE);
	}

	return pointer;
}


void* HeapManager::Alloc( int size_ )
{
	int size = roundValue( size_ );
	return static_cast<void*>( tryGetPointer( size ) );
}

void HeapManager::decommitPages( const CMemorySegment& memorySegment )
{
	__int64 first = reinterpret_cast<__int64>( memorySegment.dataPointer ) / pageSize;
	__int64 rest = reinterpret_cast<__int64>( memorySegment.dataPointer ) % pageSize;
	if( rest != 0 ) {
		first+= 1;
	}

	__int64 last = first + memorySegment.size / pageSize;

	VirtualFree(dataPointer + pageSize * first, ( last - first ) * pageSize, MEM_DECOMMIT);	
}

void HeapManager::handleNext(CMemorySegment& segment, CMemorySegment& newSegment)
{
	auto nextSegment = allSegments.find( segment.dataPointer + segment.size );
	if( nextSegment != allSegments.end() && !nextSegment->second.isBusy ) {
		newSegment.size += nextSegment->second.size;
		auto nextNextSegment = allSegments.find( newSegment.dataPointer + newSegment.size );
		if ( nextNextSegment != allSegments.end() ) {
			nextNextSegment->second.prev = newSegment.dataPointer;
		}

		allSegments.erase( nextSegment );
	}
}

void HeapManager::handlePrev(CMemorySegment& segment, CMemorySegment& newSegment)
{
	auto prevSegment = allSegments.find( segment.prev );
	if( prevSegment != allSegments.end() && !prevSegment->second.isBusy ) {
		newSegment.dataPointer = dataPointer;
		newSegment.prev = prevSegment->second.prev;
		newSegment.size += prevSegment->second.size;

		allSegments.erase( prevSegment );
	}
}

void HeapManager::Free(void* ptr)
{
	if( ptr == nullptr ) {
		return;
	}
	char* pointer = static_cast<char*>( ptr );
	auto segmentIter = allSegments.find( pointer );
	
	if (segmentIter == allSegments.end()) {
		std::cerr << ptr << " wrong pointer\n";
		return;
	}

	CMemorySegment segment = segmentIter->second;
	CMemorySegment newSegment = segment;
	
	busyPointers.erase( pointer );
	allSegments.erase( segmentIter );
	
	handleNext( segment, newSegment );
	handleNext( segment, newSegment );

	decommitPages( newSegment );

	freeSegments.insert( newSegment );
	if( newSegment.dataPointer == segment.dataPointer ) {
		allSegments[newSegment.dataPointer] = newSegment;
	} else {
		allSegments[newSegment.dataPointer] = newSegment;
	}
}

HeapManager::~HeapManager()
{
	if (busyPointers.size() != 0) {
		std::cerr << "Memory leak:\n";
		for( auto ptr : busyPointers )
		{
			if( ptr!= nullptr ) {
			std::cerr << ptr << "\n";
			}
		}
	}
	VirtualFree( dataPointer, maxSize, MEM_RELEASE );
}
