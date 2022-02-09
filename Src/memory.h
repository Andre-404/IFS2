#ifndef __IFS_MEMORY
#define __IFS_MEMORY

#include "common.h"
#include "VM.h"
#include "compiler.h"

//handpicked values, need adjusting
#define HEAP_START_SIZE (1024)
#define HEAP_MAX 1
#define COLLECTION_THRESHOLD 16384  

typedef char byte;

class GC {
public:
	GC();
	vm* VM;
	compiler* compiling;
	void* allocRaw(size_t size, bool shouldCollect);
	void clear();
private:
	//keeping track of memory allocated since last clear, as well as the point after which a allocation should occur
	long sinceLastClear;

	
	//heap -> current heap memory block
	//heapTop -> the next free pointer on the heap(NOT the top of the actual memory block)
	//oldHeap -> used when resizing the heap, objects from oldHeap get copied to the current heap, and then oldHeap is dealloced
	byte* heap;
	byte* heapTop;
	byte* oldHeap;
	size_t allocated;
	//size of the current heap memory block
	size_t heapSize;

	bool collecting;
	//used to prevent deep recursion
	std::vector<obj*> stack;

	//these are all the cached pointers to heap objects that need to be updated, works like a stack
	std::vector<obj*> cachedPtrs;

	//debug stuff
	#ifdef DEBUG_GC
	size_t nCollections;
	size_t nReallocations;
	#endif // DEBUG_GC

	void cachePtr(obj* ptr);
	obj* getCachedPtr();

	void reallocate(size_t size);
	void collect();

	void mark();
	void markObj(obj* ptr);
	void markVal(Value& val);
	void markTable(hashTable& table);
	void markRoots();
	void traceObj(obj* ptr);

	void computeAddress(bool isReallocating);

	void updatePtrs();
	void updateRootPtrs();
	void updateHeapPtrs();

	void compact();

	void moveObj(void* to, obj* from);
	void moveRaw(void* to, void* from, size_t size);

	void destructObj(obj* ptr);
};

#endif // !__IFS_MEMORY
