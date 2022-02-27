#pragma once

#include "common.h"
#include "heapBlock.h"

//handpicked values, need adjusting
#define HEAP_START_SIZE (16384*16)
#define HEAP_MAX 1


class vm;
class compiler;

//Lisp 2 style mark-compact GC,
//utilizes 2 heaps, one for small objects(defined in  object.h) and one(LOH) for (potentially)larger objects such as strings and arrays
//This was done to improve spacial locality of objs and to enable allocating arrays/strings in the constructors of obj objects
//
//if every type of data was allocated on a single heap, no allocation could happen in constructors(or methods) as it would risk moving
//the object that called the constructor/method
//Possible change: save yourself the trouble and go with a in-place GC(something akin to a Boehm-Demers-Weiser collector) for the small objs
//and then use a mark compact for arrays and strings
class GC {
public:
	GC();
	vm* VM;
	compiler* compiling;
	void* allocRaw(size_t size, bool shouldLOHAlloc);
	void clear();
	void cachePtr(managed* ptr);
	managed* getCachedPtr();
private:
	//LOH is used for arrays/strings, normal heap is used for obj* objects
	heapBlock normalHeap;
	heapBlock LOH;
	heapBlock* activeHeap;
	heapBlock* inactiveHeap;

	bool collecting;
	//used to prevent deep recursion
	std::vector<managed*> stack;

	//these are all the cached pointers to heap objects that need to be updated, works like a stack
	std::vector<managed*> cachedPtrs;

	//debug stuff
	#ifdef DEBUG_GC
	size_t nCollections;
	size_t nReallocations;
	#endif // DEBUG_GC

	void resize(size_t size);
	void collect();
	void shrink();

	void mark();
	void markObj(managed* ptr);
	void markVal(Value* val);
	void markTable(hashTable* table);
	void markRoots();
	void traceObj(managed* ptr);

	void computeAddress();

	void updatePtrs();
	void updateRootPtrs();
	void updateHeapPtrs();

	void compact();
};
