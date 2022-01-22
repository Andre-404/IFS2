#ifndef __IFS_MEMORY
#define __IFS_MEMORY

#include "common.h"
#include "VM.h"

#define HEAP_START_SIZE (2048)
#define HEAP_MIN 0.5
#define HEAP_MAX 0.8

class GC {
public:
	void collect(size_t nextAlloc);
	GC();
	void ready(vm* VM);
	bool isReady;
	void* allocRaw(size_t size, bool shouldCollect);
	void clear();
private:
	//keeping track of memory allocated since last clear, as well as the point after which a allocation should occur
	long sinceLastClear;
	long threshold;
	vm* VM;
	//heap -> current heap memory block
	//heapTop -> the next free pointer on the heap(NOT the top of the actual memory block)
	//oldHeap -> used when resizing the heap, objects from oldHeap get copied to the current heap, and then oldHeap is dealloced
	char* heap;
	char* heapTop;
	char* oldHeap;
	size_t allocated;
	//size of the current heap memory block
	size_t heapSize;

	bool collecting;
	//used to prevent deep recursion
	std::vector<obj*> stack;

	void mark();
	void markObj(obj* ptr);
	void markVal(Value& val);
	void markTable(hashTable& table);
	void markRoots();
	void traceObj(obj* ptr);

	void computeAddress();

	void updatePtrs();
	void updateRootPtrs();
	void updateHeapPtrs();

	void compact();

	void moveObj(void* to, obj* from);
	void moveRaw(void* to, void* from, size_t size);

	void destructObj(obj* ptr);
};

#endif // !__IFS_MEMORY
