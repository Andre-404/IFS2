#pragma once

#include "common.h"
#include "heapBlock.h"

//handpicked values, need adjusting
#define HEAP_START_SIZE (1024)//Starting size of both heaps are 512KB(1MB in total)


class vm;
class compiler;
class ASTNode;

//Lisp 2 style mark-compact GC,
//utilizes 2 heaps, one for small objects(defined in  object.h) and one(LOH) for (potentially)larger objects such as strings and arrays
//This was done to improve spacial locality of objs and to enable allocating arrays/strings in the constructors of obj objects
//
//if every type of data was allocated on a single heap, no allocation could happen in constructors(or methods) as it would risk moving
//the object that called the constructor/method
//It also uses a single static heap for fibers, as moving them in memory would be very hard
//Possible change(Andrea): save yourself the trouble and go with a in-place GC(something akin to a Boehm-Demers-Weiser collector) for the small objs
//and then use a mark compact for arrays and strings
class GC {
public:
	GC();
	vm* VM;
	compiler* compilerSession;
	void* allocRaw(size_t size, bool shouldLOHAlloc);
	void* allocRawStatic(size_t size);
	void clear();
	void cachePtr(managed* ptr);
	void addASTNode(ASTNode* _node) { ast.push_back(_node); }
	void clearASTNodes();
	managed* getCachedPtr();
private:
	//LOH is used for arrays/strings, normal heap is used for obj* objects
	movingHeapBlock normalHeap;
	movingHeapBlock LOH;
	movingHeapBlock* activeHeap;
	movingHeapBlock* inactiveHeap;
	staticHeapBlock staticHeap;

	bool collecting;
	//used to prevent deep recursion
	std::vector<managed*> stack;

	//these are all the cached pointers to heap objects that need to be updated, works like a stack
	std::vector<managed*> cachedPtrs;

	//gets populated during the parsing phase and cleared out after compilation is done
	std::vector<ASTNode*> ast;

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
