#pragma once

#include "common.h"
#include "managed.h"


//represents a single heap(block of memory)
//the heap doesn't handle garbage collecting by itself, rather it only keeps track of how much memory it has allocated,
//as well as a pointer to the heap
class movingHeapBlock {
public:
	movingHeapBlock();

	void* allocate(size_t size);
	bool canAllocate(size_t size);
	bool canShrink(size_t size);

	void resize(size_t size);
	void shrink();

	void clear();

	void computeAddress();
	void updatePtrs();
	void compact();
	void clearFlags();

	void dump(bool isPost);

	//heapBuffer -> current heap memory block
	//heapTop -> the next free pointer on the heap(NOT the top of the actual memory block)
	//oldHeapBuffer -> used when resizing the heap, objects from oldHeap get copied to the current heap, and then oldHeap is dealloced
	byte* heapBuffer;
	byte* oldHeapBuffer;
	byte* heapTop;

	size_t heapSize;
};


struct memBlock {
	size_t size;
	managed* block;
	memBlock() : size(0), block(nullptr) {};
	memBlock(size_t _size) : size(_size), block(reinterpret_cast<managed*>(new byte[_size])) {}
};

struct freeBlockArr {
	memBlock blocks[10];
	int count;
	memBlock operator[](size_t index) {
		return blocks[index];
	}
	freeBlockArr() : count(0) {}
};

class staticHeapBlock {
public:
	staticHeapBlock() : sinceLastClear(0) {}
	void* allocate(size_t size);
	bool shouldSweep();

	void clear();

	void updatePtrs();
	void sweep();
	void clearFlags();

	std::vector<memBlock> objects;
	freeBlockArr freeBlocks;
	size_t sinceLastClear;
};

class hashTable;
struct Value;

void updateTable(hashTable* table);
void updateVal(Value* val);