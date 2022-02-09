#include "memory.h"
#include "namespaces.h"
#include <immintrin.h>

GC::GC() {
	collecting = false;
	allocated = 0;
	sinceLastClear = allocated;

	heap = new byte[HEAP_START_SIZE];
	heapTop = heap;
	oldHeap = nullptr;
	heapSize = HEAP_START_SIZE;
	

	VM = nullptr;
	compiling = nullptr;

	//debug stuff
	#ifdef DEBUG_GC
	nCollections = 0;
	nReallocations = 0;
	#endif // DEBUG_GC
}

//these 2 template function are used for actually moving the objects in memory
//we're explicitly calling the destructors to get rid of any STL containers that are a part of the object
template<typename T> 
void moveData(void* to, T* from) {
	T temp = *from;
	from->~T();
	T* test = new(to) T(temp);
}

template<typename T> 
void destruct(T* ptr) {
	ptr->~T();
}

//we call this in the overwritten new operator of obj
//it should also be used explicitly for allocating header + raw data(strings, arrays)
void* GC::allocRaw(size_t size, bool shouldCollect) {
	#ifdef DEBUG_STRESS_GC
	collect();
	#endif // DEBUG_STRESS_GC

	//we only collect/reallocate if we're full
	double percentage = (double)(allocated + size) / (double)heapSize;
	if (percentage > HEAP_MAX) {
		//we first collect, hoping to free up some space on the heap
		collect();
	}
	percentage = (double)(allocated + size) / (double)heapSize;
	//if we're still missing space, we reallocate the entire heap
	//we use a while loop here in case the new heap size still isn't enough
	if(percentage > HEAP_MAX) {
		size_t newHeapSize = (1ll << (64 - _lzcnt_u64(allocated + size - 1)));
		reallocate(newHeapSize);
	}
	void* temp = heapTop;
	heapTop += size;
	allocated += size;
	return temp;
}

//while most objects simply return sizeof, some(which serve mearly as headers for some data) require additional work to figure out the size
size_t getSizeOfObj(obj* ptr) {
	switch (ptr->type) {
	case OBJ_ARRAY: return sizeof(objArray); break;
	case OBJ_CLOSURE: return sizeof(objClosure); break;
	case OBJ_FUNC: return sizeof(objFunc); break;
	case OBJ_NATIVE: return sizeof(objNativeFn); break;
	case OBJ_STRING: 
		return sizeof(objString) + ((objString*)ptr)->length + 1; break;
	case OBJ_UPVALUE: return sizeof(objUpval); break;
	case OBJ_ARR_HEADER: {
		objArrayHeader* header = (objArrayHeader*)ptr;
		return sizeof(objArrayHeader) + (header->capacity * sizeof(Value));
	}
	case OBJ_CLASS: return sizeof(objClass); break;
	case OBJ_INSTANCE: return sizeof(objInstance); break;
	case OBJ_BOUND_METHOD: return sizeof(objBoundMethod); break;
	case OBJ_ITERATOR: return sizeof(objIterator); break;
	default:
		std::cout << "Couldn't convert obj" << "\n";
		exit(1);
	}
}

void GC::collect() {
	if (collecting || (VM == nullptr && compiling == nullptr)) return;
	collecting = true;
	oldHeap = heap;
	mark();
	//calculate the address of each marked object after compaction
	computeAddress(false);
	//update pointers both in the roots and in the heap objects themselves,
	//special care needs to be taken to ensure that there are no cached pointers prior to a collection happening
	updatePtrs();
	//sliding objects in memory, the heap memory buffer "to" is the same as "from"
	compact();
	oldHeap = nullptr;
	sinceLastClear = allocated;
	collecting = false;

	//debug stuff
	#ifdef DEBUG_GC
	nCollections ++;
	#endif // DEBUG_GC
}

void GC::reallocate(size_t size) {
	if (collecting || (VM == nullptr && compiling == nullptr)) return;
	collecting = true;
	oldHeap = heap;
	heapSize = size;//heap size is always a power of 2, maybe change this?
	heap = new byte[heapSize];
	//calculate the address of each marked object after reallocation
	computeAddress(true);
	//update pointers both in the roots and in the heap objects themselves,
	//special care needs to be taken to ensure that there are no cached pointers prior to a collection happening
	updatePtrs();
	//copying object to a new memory buffer
	compact();
	delete[] oldHeap;
	oldHeap = nullptr;
	sinceLastClear = allocated;
	collecting = false;
	//debug stuff
	#ifdef DEBUG_GC
	nReallocations++;
	#endif // DEBUG_GC
}


void GC::markVal(Value& val) {
	if (val.type == VAL_OBJ) markObj(AS_OBJ(val));
}

void setMarked(obj* ptr) {
	ptr->moveTo = ptr;//a non null pointer means this is a marked object
}

void GC::markObj(obj* ptr) {
	if (!ptr) return;
	//there is no reason to push native function and strings to the stack, we can't trace any further nodes from them
	if (ptr->type == OBJ_NATIVE || ptr->type == OBJ_STRING) {
		setMarked(ptr);
		return;
	}
	stack.push_back(ptr);
	
}

void GC::markTable(hashTable& table) {
	for (int i = 0; i < table.capacity; i++) {
		entry* _entry = &table.entries[i];
		if (_entry->key && _entry->key != TOMBSTONE) {
			markObj(_entry->key);
			markVal(_entry->val);
		}
	}
}

void GC::markRoots() {
	if (VM != nullptr) {
		for (Value* val = VM->stack; val < VM->stackTop; val++) {
			markVal(*val);
		}
		markTable(VM->globals);
		for (int i = 0; i < VM->frameCount; i++) {
			markObj(VM->frames[i].closure);
		}
		for (int i = 0; i < VM->openUpvals.size(); i++) {
			markObj(VM->openUpvals[i]);
		}
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
	for (obj* ptr : cachedPtrs) {
		markObj(ptr);
	}
	//we use a stack to avoid going into a deep recursion(which might fail)
	while (stack.size() != 0) {
		obj* ptr = stack.back();
		stack.pop_back();
		traceObj(ptr);
	}
}

void GC::traceObj(obj* object) {
	if (!object) return;
	setMarked(object);
	switch (object->type) {
	case OBJ_FUNC: {
		objFunc* func = (objFunc*)object;
		stack.push_back(func->name);
		for (Value val : func->body.constants) {
			markVal(val);
		}
		break;
	}
	case OBJ_ARRAY: {
		objArray* arr = (objArray*)object;
		stack.push_back(arr->values);
		break;
	}
	case OBJ_CLOSURE: {
		objClosure* closure = (objClosure*)object;
		stack.push_back(closure->func);
		for (objUpval* upval : closure->upvals) {
			stack.push_back(upval);
		}
		break;
	}
	case OBJ_UPVALUE: markVal(((objUpval*)object)->closed);  break;
	case OBJ_ARR_HEADER: {
		objArrayHeader* header = (objArrayHeader*)object;
		for (size_t i = 0; i < header->count; i++) {
			markVal(header->arr[i]);
		}
		break;
	}
	case OBJ_CLASS: {
		objClass* klass = (objClass*)object;
		klass->name->moveTo = object;
		markTable(klass->methods);
		break;
	}
	case OBJ_INSTANCE: {
		objInstance* inst = (objInstance*)object;
		markTable(inst->table);
		if (inst->klass != nullptr) markObj(inst->klass);
		break;
	}
	case OBJ_BOUND_METHOD: {
		objBoundMethod* bound = (objBoundMethod*)object;
		markVal(bound->receiver);
		break;
	}
	case OBJ_ITERATOR: {
		objIterator* iterator = (objIterator*)object;
		markObj(iterator->iteratable);
		break;
	}
	case OBJ_NATIVE:
	case OBJ_STRING: setMarked(object); break;
	}
}



bool forwardAddress(obj* ptr) {
	return ptr->moveTo != nullptr;
}

void GC::computeAddress(bool isReallocating) {
	//we scan the heap linearly, for each marked object(those whose moveTo field isn't null), we calculate a new(compacted) position
	//"to" can point either to start of the same memory block, or to a entirely memory block
	byte* to = heap;
	byte* from = oldHeap;

	while (from < heapTop) {
		obj* temp = (obj*)from;
		if (isReallocating || forwardAddress((obj*)from)) {
			temp->moveTo = (obj*)to;
			//move the compacted position pointer
			to += getSizeOfObj(temp);
		}
		//get the next object from old heap
		from += getSizeOfObj(temp);

	}
}


void updateVal(Value* val) {
	if (val->type == VAL_OBJ) val->as.object = val->as.object->moveTo;
}

void updateTable(hashTable* table) {
	for (int i = 0; i < table->capacity; i++) {
		entry* _entry = &table->entries[i];
		//only update live cells(avoid empty/tombstone ones)
		if (_entry->key && _entry->key != TOMBSTONE) {
			//this little check here is for global::internedStrings, since the strings there are "weak" pointers
			if (_entry->key->moveTo == nullptr) {
				table->del(_entry->key);
			}
			else {
				_entry->key = (objString*)_entry->key->moveTo;
				updateVal(&_entry->val);
			}
		}
	}
}

//handling the internal pointers of every object type
void updateObjectPtrs(obj* object) {
	switch (object->type) {
	case OBJ_FUNC: {
		objFunc* func = (objFunc*)object;
		if(func->name != nullptr) func->name = (objString*)func->name->moveTo;
		int size = func->body.constants.size();
		for (int i = 0; i < size; i++) {
			updateVal(&func->body.constants[i]);
		}
		break;
	}
	case OBJ_CLOSURE: {
		objClosure* closure = (objClosure*)object;
		closure->func = (objFunc*)closure->func->moveTo;
		long size = closure->upvals.size();
		for (int i = 0; i < size; i++) {
			if(closure->upvals[i] != nullptr) closure->upvals[i] = (objUpval*)closure->upvals[i]->moveTo;
		}
		break;
	}
	case OBJ_ARRAY: {
		objArray* arr = (objArray*)object;
		arr->values = (objArrayHeader*)arr->values->moveTo;
		break;
	}
	case OBJ_UPVALUE: {
		objUpval* upval = (objUpval*)object;
		if (!upval->isOpen) {
			updateVal(&upval->closed);
		}
		break;
	}
	case OBJ_STRING: {
		objString* str = (objString*)object;
		//make sure to update the pointer to the char* string, it's always going to be located right after the header
		str->str = (char*)str->moveTo + sizeof(objString);
		break;
	}
	case OBJ_ARR_HEADER: {
		objArrayHeader* header = (objArrayHeader*)object;

		size_t size = header->count;
		for (size_t i = 0; i < size; i++) {
			updateVal(&header->arr[i]);
		}

		header->arr = (Value*)(((byte*)header) + sizeof(objArrayHeader));
		break;
	}
	case OBJ_CLASS:{
		objClass* klass = (objClass*)object;
		klass->name = (objString*)klass->name->moveTo;
		updateTable(&klass->methods);
		break;
	}
	case OBJ_INSTANCE: {
		objInstance* inst = (objInstance*)object;
		if(inst->klass != nullptr) inst->klass = (objClass*)inst->klass->moveTo;
		updateTable(&inst->table);
		break;
	}
	case OBJ_BOUND_METHOD: {
		objBoundMethod* method = (objBoundMethod*)object;
		method->method = (objClosure*)method->method->moveTo;
		updateVal(&method->receiver);
		break;
	}
	case OBJ_ITERATOR: {
		objIterator* iterator = (objIterator*)object;
		iterator->iteratable = iterator->iteratable->moveTo;
		break;
	}
	case OBJ_NATIVE:
		break;
	}
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
		for (Value* val = VM->stack; val < VM->stackTop; val++) {
			updateVal(val);
		}
		updateTable(&VM->globals);
		for (int i = 0; i < VM->frameCount; i++) {
			VM->frames[i].closure = (objClosure*)VM->frames[i].closure->moveTo;
		}
		for (int i = 0; i < VM->openUpvals.size(); i++) {
			VM->openUpvals[i] = (objUpval*)VM->openUpvals[i]->moveTo;
		}
	}
	//the GC can also be called during the compilation process, so we need to update it's compiler info
	if (compiling != nullptr) {
		compilerInfo* cur = compiling->current;
		while (cur != nullptr) {
			cur->func = (objFunc*)cur->func->moveTo;
			cur->code = &cur->func->body;
			cur = cur->enclosing;
		}
	}
}

void GC::updateHeapPtrs() {
	//sort of like marking, we scan the heap lineraly, and for each marked object we update it's pointers
	char* current = oldHeap;

	while (current < heapTop) {
		obj* temp = (obj*)current;
		if (forwardAddress(temp)) {
			updateObjectPtrs(temp);
		}
		current += getSizeOfObj(temp);
	}
}


void GC::compact() {
	//heap can be either the same memory buffer, or a completely different one
	//we're always copy FROM old heap
	byte* from = oldHeap;
	byte* newTop = heap;

	//heapTop points to the top of the old heap, we ONLY update heapTop once we're done with compacting
	while (from < heapTop) {
		obj* curObject = (obj*)from;
		size_t sizeOfObj = getSizeOfObj(curObject);
		byte* nextObj = from + sizeOfObj;

		if (forwardAddress(curObject)) {
			byte* to = (byte*)curObject->moveTo;
			curObject->moveTo = nullptr;//reset the marked flag
			//this is a simple optimization, if the object doesn't move in memory at all, there's no need to copy it
			if(from != to) moveObj(to, curObject);
			newTop = to + sizeOfObj;
		}else {
			allocated -= sizeOfObj;
			//destructObj MUST be called in order to properly handle the destruction of STL containers(like vectors)
			destructObj(curObject);
		}
		from = nextObj;
	}
	//only update heapTop once we're done compacting
	heapTop = newTop;
}


void GC::moveObj(void* to, obj* from) {
	switch (from->type) {
	case OBJ_ARRAY:		moveData(to, (objArray*)from); break;
	case OBJ_STRING: {
		//in this case, objString serves as a header for the char* that comes after it, this spacial locality MUST be maintianed
		//to do this, we first copy the header, and then moveRaw(which is just memcpy under the hood) to the pointer after the header
		moveData(to, (objString*)from);
		objString* str = (objString*)to;
		//length + 1 here is for the null terminator
		moveRaw((byte*)to + sizeof(objString), (byte*)from + sizeof(objString), ((objString*)from)->length + 1);
		break;
	}
	case OBJ_UPVALUE: {
		moveData(to, (objUpval*)from);
		//when moving a upvalue, if it's closed we have to update it's location pointer to point to a new location of "closed"
		objUpval* upval = (objUpval*)to;
		if (!upval->isOpen) upval->location = &upval->closed;
		break;
	}
	//all of these objects can be moved by simply copying them(meaning they don't serve as headers for other data,
	//nor do they have cached pointers to data other than heap objects)
	case OBJ_CLOSURE:	moveData(to, (objClosure*)from); break;
	case OBJ_FUNC:		moveData(to, (objFunc*)from); break;
	case OBJ_NATIVE:	moveData(to, (objNativeFn*)from); break;
	case OBJ_ARR_HEADER: {
		size_t count = ((objArrayHeader*)from)->count;
		size_t capacity = ((objArrayHeader*)from)->capacity;
		moveData(to, (objArrayHeader*)from);
		objArrayHeader* newHeader = (objArrayHeader*)to;
		//newHeader->arr = (Value*)(((char*)newHeader) + sizeof(objArrayHeader));
		Value* fromArr = ((Value*)(((byte*)from + sizeof(objArrayHeader))));
		memmove((byte*)newHeader->arr, (byte*)fromArr, capacity * sizeof(Value));
		break;
	}
	case OBJ_CLASS: moveData(to, (objClass*)from); break;
	case OBJ_INSTANCE: moveData(to, (objInstance*)from); break;
	case OBJ_BOUND_METHOD: moveData(to, (objBoundMethod*)from); break;
	case OBJ_ITERATOR: moveData(to, (objIterator*)from); break;
	}
}

void GC::moveRaw(void* to, void* from, size_t size) {
	memcpy(to, from, size);
}

//call the destructor for the appropriate type
void GC::destructObj(obj* ptr) {
	switch (ptr->type) {
	case OBJ_ARRAY:			destruct((objArray*)ptr); break;
	case OBJ_STRING:		destruct((objString*)ptr); break;
	case OBJ_CLOSURE:		destruct((objClosure*)ptr); break;
	case OBJ_FUNC:			destruct((objFunc*)ptr); break;
	case OBJ_NATIVE:		destruct((objNativeFn*)ptr); break;
	case OBJ_UPVALUE:		destruct((objUpval*)ptr); break;
	case OBJ_CLASS:			destruct((objClass*)ptr); break;
	case OBJ_INSTANCE:		destruct((objInstance*)ptr); break;
	case OBJ_BOUND_METHOD:	destruct((objBoundMethod*)ptr); break;
	case OBJ_ITERATOR:		destruct((objIterator*)ptr); break;
	}
}


//helpers

//deallocates the entire heap
void GC::clear() {
	byte* it = heap;

	//we need to explicitly call the destructor for each object to handle things like STL containers
	while (it < heapTop) {
		obj* temp = (obj*)it;
		size_t size = getSizeOfObj(temp);
		destructObj(temp);
		it += size;

	}

	delete[] heap;
	heap = nullptr;
	heapTop = nullptr;
	oldHeap = nullptr;
	//debug stuff
	#ifdef DEBUG_GC
	std::cout << "GC ran " << nCollections << " times\n";
	std::cout << "GC realloacted the heap " << nReallocations << " times\n";
	std::cout << "Final size of heap: " << heapSize << "\n";
	#endif // DEBUG_GC
	
}

//these 2 functions are for handling potential cached pointers(such pointers may appear in native functions/while executing bytecode)
void GC::cachePtr(obj* ptr) {
	cachedPtrs.push_back(ptr);
}

obj* GC::getCachedPtr() {
	if (cachedPtrs.size() == 0) {
		std::cout << "Cannot retrieve cached pointer, exiting...\n";
		exit(64);
	}
	obj* ptr = cachedPtrs[cachedPtrs.size() - 1];
	cachedPtrs.pop_back();
	return ptr;
}
