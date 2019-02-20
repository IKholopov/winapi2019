#include <iostream>
#include <Windows.h>
#include <random>
#include <algorithm>
#include <chrono>

#include "HeapManager.h"



void simpleTest( bool isCustom )
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> uid(10, 50);

	std::vector<char*> ptrs;
	HeapManager heapManager( 4 * 1024, 1000 * 1024 * 1024 );

	int length = 100000;

	auto startTime = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < 3 * length; i++) {
		if(isCustom) {
			ptrs.push_back(static_cast<char*>( heapManager.Alloc( uid( gen ) ) ) );
		} else {
			ptrs.push_back( new char[uid( gen ) ] );
		}
	}

	for (size_t i = 0; i < 3 * length; i++) {
		if (isCustom) {
			auto i = ptrs.back();
			heapManager.Free( i );
		}
		else {
			delete[] ptrs.back();
		}

		ptrs.pop_back();
	}
	

	auto endTime = std::chrono::high_resolution_clock::now();

	std::cout<< ( isCustom ? "Custom Allocator " : "Standart Allocator " ) << ( endTime - startTime ) / std::chrono::milliseconds( 1 ) << "ms\n";
}

void allocFreeTest( bool isCustom )
{
	std::random_device rd;
	std::mt19937 gen( rd() );
	std::uniform_int_distribution<> uid( 70, 100 );

	std::vector<char*> ptrs;
	HeapManager heapManager( 4 * 1024, 200 * 1024 * 1024 );

	int length = 100000;

	auto startTime = std::chrono::high_resolution_clock::now();

	for ( size_t i = 0; i < 3 * length; i++ ) {
		if ( isCustom ) {
			ptrs.push_back( static_cast<char*>( heapManager.Alloc( uid( gen ) ) ) );
		}
		else {
			ptrs.push_back( new char[uid( gen )] );
		}
	}

	std::shuffle( ptrs.begin(), ptrs.end(), gen);


	for ( size_t i = 0; i < length; i++ ) {
		if ( isCustom ) {
			auto i = ptrs.back();
			heapManager.Free( i );
		}
		else {
			delete[] ptrs.back();
		}

		ptrs.pop_back();
	}

	for (size_t i = 0; i < length; i++) {
		if ( isCustom ) {
			ptrs.push_back( static_cast<char*>( heapManager.Alloc( uid( gen ) ) ) );
		}
		else {
			ptrs.push_back( new char[uid( gen ) ] );
		}
	}

	std::shuffle( ptrs.begin(), ptrs.end(), gen );

	for (size_t i = 0; i < 2 * length; i++) {
		if (isCustom) {
			auto i = ptrs.back();
			heapManager.Free( i );
		}
		else {
			delete[] ptrs.back();
		}

		ptrs.pop_back();
	}
	
	for (size_t i = 0; i < length; i++) {
		if ( isCustom ) {
			ptrs.push_back( static_cast<char*>( heapManager.Alloc( uid( gen ) ) ) );
		}
		else {
			ptrs.push_back( new char[uid( gen ) ] );
		}
	}
	
	std::shuffle( ptrs.begin(), ptrs.end(), gen );

	for ( size_t i = 0; i < 2 * length; i++ ) {
		if( isCustom ) {
			auto i = ptrs.back();
			heapManager.Free(i);
		}
		else {
			delete[] ptrs.back();
		}

		ptrs.pop_back();
	}

	auto endTime = std::chrono::high_resolution_clock::now();

	std::cout <<  ( isCustom ? "Custom Allocator " : "Standart Allocator " ) << ( endTime - startTime ) / std::chrono::milliseconds( 1 ) << "ms\n";
}

void simpleTestBig(bool isCustom)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> uid( 1024, 5 * 1024 );

	std::vector<char*> ptrs;
	HeapManager heapManager(4 * 1024, 1000 * 1024 * 1024 );

	int length = 100000;

	auto startTime = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < 3 * length; i++) {
		if (isCustom) {
			ptrs.push_back(static_cast<char*>(heapManager.Alloc(uid(gen))));
		}
		else {
			ptrs.push_back(new char[uid(gen)]);
		}
	}

	for (size_t i = 0; i < 3 * length; i++) {
		if (isCustom) {
			auto i = ptrs.back();
			heapManager.Free(i);
		}
		else {
			delete[] ptrs.back();
		}

		ptrs.pop_back();
	}


	auto endTime = std::chrono::high_resolution_clock::now();

	std::cout << (isCustom ? "Custom Allocator " : "Standart Allocator ") << (endTime - startTime) / std::chrono::milliseconds(1) << "ms\n";
}

void allocFreeTestDifferent( bool isCustom )
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> uid(5, 5 * 1024);

	std::vector<char*> ptrs;
	HeapManager heapManager(4 * 1024, 1000 * 1024 * 1024);

	int length = 100000;

	auto startTime = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < 3 * length; i++) {
		if (isCustom) {
			ptrs.push_back(static_cast<char*>(heapManager.Alloc(uid(gen))));
		}
		else {
			ptrs.push_back(new char[uid(gen)]);
		}
	}

	std::shuffle(ptrs.begin(), ptrs.end(), gen);

	for (size_t i = 0; i < length; i++) {
		if (isCustom) {
			auto i = ptrs.back();
			heapManager.Free(i);
		}
		else {
			delete[] ptrs.back();
		}

		ptrs.pop_back();
	}

	for (size_t i = 0; i < length; i++) {
		if (isCustom) {
			ptrs.push_back(static_cast<char*>(heapManager.Alloc(uid(gen))));
		}
		else {
			ptrs.push_back(new char[uid(gen)]);
		}
	}

	std::shuffle(ptrs.begin(), ptrs.end(), gen);

	for (size_t i = 0; i < 3 * length; i++) {
		if (isCustom) {
			auto i = ptrs.back();
			heapManager.Free(i);
		}
		else {
			delete[] ptrs.back();
		}

		ptrs.pop_back();
	}
	auto endTime = std::chrono::high_resolution_clock::now();

	std::cout << (isCustom ? "Custom Allocator " : "Standart Allocator ") << (endTime - startTime) / std::chrono::milliseconds(1) << "ms\n";
}

int main()
{
	/*simpleTest( true );
	simpleTest( false );

	allocFreeTest( true );
	allocFreeTest( false );
	
	simpleTestBig( true );
	simpleTestBig( false );
	*/
	
	allocFreeTestDifferent( true );
	allocFreeTestDifferent( false );
	
	return 0;
}