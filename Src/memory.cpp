#include "memory.h"
#include "namespaces.h"
#include "VM.h"
#include "compiler.h"
#include <immintrin.h>

GC::GC() {
	collecting = false;
	
	activeHeap = nullptr;
	inactiveHeap = nullptr;

	VM = nullptr;
	compiling = nullptr;

	//debug stuff
	#ifdef DEBUG_GC
	nCollections = 0;
	nReallocations = 0;
	#endif // DEBUG_GC
}

//we call this in the overwritten new operator of obj
//it should also be used explicitly for allocating header + raw data(strings, arrays), this is allocated on Large Object Heap(LOH)
void* GC::allocRaw(size_t size, bool shouldLOHAlloc) {
	activeHeap = shouldLOHAlloc ? &LOH : &normalHeap;
	inactiveHeap = shouldLOHAlloc ? &normalHeap : &LOH;

	#ifdef DEBUG_STRESS_GC
	std::cout << "GC ran ";
	if (activeHeap == &LOH) std::cout << "LOH\n";
	else std::cout << "normal\n";
	collect();
	#endif // DEBUG_STRESS_GC

	//we only collect/resize if we're full
	if (!activeHeap->canAllocate(size)) {
		//we first collect, hoping to free up some space on the heap
		#ifdef DEBUG_GC
		std::cout << "GC ran ";
		if (activeHeap == &LOH) std::cout << "LOH\n";
		else std::cout << "normal\n";
		#endif // DEBUG_GC
		collect();
	}
	//if we're still missing space, we reallocate the entire heap
	//we use a while loop here in case the new heap size still isn't enough
	if(!activeHeap->canAllocate(size)) {
		resize(size);
	}else if (activeHeap->canShrink(size)) shrink();
	
	return activeHeap->allocate(size);
}

void* GC::allocRawStatic(size_t size) {
	if (staticHeap.shouldSweep()) {
		mark();
		staticHeap.sweep();
		LOH.clearFlags();
		normalHeap.clearFlags();
	}
	return staticHeap.allocate(size);
}

void GC::collect() {
	if (collecting || (VM == nullptr && compiling == nullptr)) return;
	collecting = true;
	mark();
	//calculate the address of each marked object after compaction
	computeAddress();
	//update pointers both in the roots and in the heap objects themselves,
	//special care needs to be taken to ensure that there are no cached pointers prior to a collection happening
	updatePtrs();
	//sliding objects in memory, the heap memory buffer "to" is the same as "from"
	compact();
	activeHeap->oldHeapBuffer = nullptr;
	collecting = false;

	//debug stuff
	#ifdef DEBUG_GC
	nCollections ++;
	#endif // DEBUG_GC
}

void GC::resize(size_t size) {
	if (collecting || (VM == nullptr && compiling == nullptr)) return;
	collecting = true;

	activeHeap->resize(size);

	mark();
	//calculate the address of each marked object after reallocation
	computeAddress();
	//update pointers both in the roots and in the heap objects themselves,
	//special care needs to be taken to ensure that there are no cached pointers prior to a collection happening
	updatePtrs();
	//copying object to a new memory buffer while also compacting the heap
	compact();
	delete[] activeHeap->oldHeapBuffer;
	activeHeap->oldHeapBuffer = nullptr;
	collecting = false;
	//debug stuff
	#ifdef DEBUG_GC
	nReallocations++;
	#endif // DEBUG_GC
}

void GC::shrink() {
	if (collecting || (VM == nullptr && compiling == nullptr)) return;
	collecting = true;
	activeHeap->shrink();

	mark();
	//calculate the address of each marked object after reallocation
	computeAddress();
	//update pointers both in the roots and in the heap objects themselves,
	//special care needs to be taken to ensure that there are no cached pointers prior to a collection happening
	updatePtrs();
	//copying object to a new memory buffer while also compacting the heap
	compact();
	delete[] activeHeap->oldHeapBuffer;
	activeHeap->oldHeapBuffer = nullptr;
	collecting = false;
	//debug stuff
	#ifdef DEBUG_GC
	nReallocations++;
	#endif // DEBUG_GC
}


void GC::markVal(Value* val) {
	if (IS_OBJ(*val)) markObj(AS_OBJ(*val));
}

void setMarked(managed* ptr) {
	ptr->moveTo = ptr;//a non null pointer means this is a marked object
}

void GC::markObj(managed* ptr) {
	if (!ptr) return;
	stack.push_back(ptr);
	
}

void GC::markTable(hashTable* table) {
	for (int i = 0; i < table->capacity; i++) {
		entry* _entry = &table->entries[i];
		if (_entry->key && _entry->key != TOMBSTONE) {
			markObj(_entry->key);
			markVal(&_entry->val);
		}
	}
	table->entries.mark();
}

void GC::markRoots() {
	global::internedStrings.entries.mark();
	if (VM != nullptr) {
		markObj(VM->curFiber);
		markTable(&VM->globals);
	}
	//since a GC collection can be triggered during the compilation, we must also mark all function which are currently being compiled
	if (compiling != nullptr) {
		compilerInfo* cur = compiling->current;
		while (cur != nullptr) {
			markObj(cur->func);
			cur = cur->enclosing;
		}
	}
}

void GC::mark() {
	markRoots();
	//cached pointers are also considered root nodes(since such cached pointers may be a direct result of things like the pop() operation on the stack)
	for (managed* ptr : cachedPtrs) {
		markObj(ptr);
	}
	//we use a stack to avoid going into a deep recursion(which might fail)
	while (stack.size() != 0) {
		managed* ptr = stack.back();
		stack.pop_back();
		traceObj(ptr);
	}
}

void GC::traceObj(managed* object) {
	if (!object || object->moveTo != nullptr) return;
	setMarked(object);
	object->trace(stack);
}

void GC::computeAddress() {
	activeHeap->computeAddress();
}

//handling the internal pointers of every object type
void updateObjectPtrs(managed* object) {
	object->updatePtrs();
}

void GC::updatePtrs() {
	updateRootPtrs();
	updateHeapPtrs();
	//updating cached pointers
	for (int i = 0; i < cachedPtrs.size(); i++) {
		cachedPtrs[i] = cachedPtrs[i]->moveTo;
	}
}

void GC::updateRootPtrs() {
	//we update the interned strings, but don't mark them, that's because the strings in this hash map are "weak" pointers
	//if no reference to a string exists, there's no point in keeping it in memory
	updateTable(&global::internedStrings);
	if (VM != nullptr) {
		if(VM->curFiber != nullptr) VM->curFiber = reinterpret_cast<objFiber*>(VM->curFiber->moveTo);
		updateTable(&VM->globals);
	}
	//the GC can also be called during the compilation process, so we need to update it's compiler info
	if (compiling != nullptr) {
		compilerInfo* cur = compiling->current;
		while (cur != nullptr) {
			cur->func = reinterpret_cast<objFunc*>(cur->func->moveTo);
			cur = cur->enclosing;
		}
	}
}

void GC::updateHeapPtrs() {
	normalHeap.updatePtrs();
	LOH.updatePtrs();
	staticHeap.updatePtrs();
}


void GC::compact() {
	activeHeap->compact();
	inactiveHeap->clearFlags();
	staticHeap.clearFlags();
}


//helpers

//deallocates the entire heap
void GC::clear() {
	normalHeap.clear();
	LOH.clear();

	//debug stuff
	#ifdef DEBUG_GC
	std::cout << "GC ran " << nCollections << " times\n";
	std::cout << "GC realloacted the heap " << nReallocations << " times\n";
	#endif // DEBUG_GC
	
}

//these 2 functions are for handling potential cached pointers(such pointers may appear in native functions/while executing bytecode)
void GC::cachePtr(managed* ptr) {
	cachedPtrs.push_back(ptr);
}

managed* GC::getCachedPtr() {
	if (cachedPtrs.size() == 0) {
		std::cout << "Cannot retrieve cached pointer, exiting...\n";
		exit(64);
	}
	managed* ptr = cachedPtrs[cachedPtrs.size() - 1];
	cachedPtrs.pop_back();
	return ptr;
}

void GC::clearASTNodes() {
	for (ASTNode* node : ast) {
		delete node;
	}
	ast.clear();
}