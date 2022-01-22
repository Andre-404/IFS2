#include "memory.h"
#include "namespaces.h"

GC::GC() {
	collecting = false;
	isReady = false;
	threshold = 512;
	VM = NULL;
	heap = new char[HEAP_START_SIZE];
	heapTop = heap;
	oldHeap = nullptr;
	allocated = 0;
	heapSize = HEAP_START_SIZE;
	sinceLastClear = allocated;
}

//these 2 template function are used for actually moving the objects in memory
//we're explicitly calling the destructors to get rid of any containers that are a part of the object
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
//it should also be used for allocating header + raw data(strings, arrays)
void* GC::allocRaw(size_t size, bool shouldCollect) {
	if (shouldCollect) collect(size);
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

	default:
		std::cout << "Couldn't convert obj" << "\n";
		exit(1);
	}
}

//deallocts the entire heap
void GC::clear() {
	char* it = heap;

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
}

//the GC won't collect any objects before this is called, could maybe switch it to work with VM and a compiler?
void GC::ready(vm* _VM) {
	VM = _VM;
	isReady = true;
}

void GC::collect(size_t nextAlloc) {
	if (collecting || VM == NULL) return;
	collecting = true;
	//we must account for the next alloc that's about to happen when determining whether or not to resize the heap
	long delta = allocated - sinceLastClear + nextAlloc;
	size_t topOfHeap = heapTop - heap + nextAlloc;
	double percentage = (double)topOfHeap / (double)heapSize;
	if (delta < threshold && percentage < HEAP_MIN) {
		collecting = false;
		return;
	}
	oldHeap = heap;
	//if we're indeed resizing the heap, we combine it with the compacting(to avoid multiple passes)
	bool hasReallocated = false;
	if (percentage > HEAP_MAX) {
		heapSize = heapSize * 2;//heap size is always a power of 2, maybe change this?
		heap = new char[heapSize];
		hasReallocated = true;
	}
	mark();
	//calculate the address of each marked object after compaction
	computeAddress();
	//update pointers both in the roots and in the heap objects themselves,
	//special care needs to be taken to ensure that there are no cached pointers prior to a collection happening
	updatePtrs();
	//sliding objects in memory(or copying them to a new buffer if we're resizing the heap)
	compact();
	if(hasReallocated) delete[] oldHeap;
	sinceLastClear = allocated;
	collecting = false;
}

void GC::markVal(Value& val) {
	if (val.type == VAL_OBJ) markObj(AS_OBJ(val));
}

void setMarked(obj* ptr) {
	ptr->moveTo = ptr;//a non null pointer means this is a marked object
}

void GC::markObj(obj* ptr) {
	if (!ptr) return;
	//there is no reason to push native function and strings to the stack, we can't trace any further nodes from the,
	if (ptr->type == OBJ_NATIVE || ptr->type == OBJ_STRING) {
		setMarked(ptr);
		return;
	}
	stack.push_back(ptr);
	
}

void GC::markTable(hashTable& table) {
	for (int i = 0; i < table.capacity; i++) {
		entry* _entry = &table.entries[i];
		if (_entry->key) {
			markObj(_entry->key);
			markVal(_entry->val);
		}
	}
}

void GC::markRoots() {
	if (VM != NULL) {
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
}

void GC::mark() {
	markRoots();
	//we use a stack to avoid going into a deep recursion(which might fail)
	while (stack.size() != 0) {
		obj* ptr = stack.back();
		stack.pop_back();
		traceObj(ptr);
	}
}

void GC::traceObj(obj* object) {
	if (!object) return;
	switch (object->type) {
	case OBJ_FUNC: {
		objFunc* func = (objFunc*)object;
		stack.push_back(func->name);
		for (Value val : func->body.constants) {
			markVal(val);
		}
		setMarked(func);
		break;
	}
	case OBJ_ARRAY: {
		objArray* arr = (objArray*)object;
		setMarked(arr);
		for (Value val : arr->values) {
			markVal(val);
		}
		break;
	}
	case OBJ_CLOSURE: {
		objClosure* closure = (objClosure*)object;
		stack.push_back(closure->func);
		for (objUpval* upval : closure->upvals) {
			stack.push_back(upval);
		}
		setMarked(closure);
		break;
	}
	case OBJ_UPVALUE: setMarked(object); markVal(((objUpval*)object)->closed);  break;
	case OBJ_NATIVE:
	case OBJ_STRING: setMarked(object); break;
	}
}



bool forwardAddress(obj* ptr) {
	return ptr->moveTo != nullptr;
}

void GC::computeAddress() {
	//we scan the heap linearly, for each marked object(those whose moveTo field isn't null), we calculate a new(compacted) position
	//"to" can point either to start of the same memory, or to a entirely new buffer
	char* to = heap;
	char* from = oldHeap;

	while (from < heapTop) {
		obj* temp = (obj*)from;
		if (forwardAddress((obj*)from)) {
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
		if (_entry->key && _entry->key != (objString*)0x000001) {
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
			closure->upvals[i] = (objUpval*)closure->upvals[i]->moveTo;
		}
		break;
	}
	case OBJ_ARRAY: {
		objArray* arr = (objArray*)object;
		size_t size = arr->values.size();
		for (size_t i = 0; i < size; i++) {
			updateVal(&arr->values[i]);
		}
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
	case OBJ_NATIVE:
		break;
	}
}

void GC::updatePtrs() {
	updateRootPtrs();
	updateHeapPtrs();
}

void GC::updateRootPtrs() {
	if (VM != NULL) {
		for (Value* val = VM->stack; val < VM->stackTop; val++) {
			updateVal(val);
		}
		updateTable(&VM->globals);
		//we update the interned strings, but don't mark them, that's because the strings in this hash map are "weak" pointers
		//if no reference to a string exists, there's no point in keeping it in memory
		updateTable(&global::internedStrings);
		for (int i = 0; i < VM->frameCount; i++) {
			VM->frames[i].closure = (objClosure*)VM->frames[i].closure->moveTo;
		}
		for (int i = 0; i < VM->openUpvals.size(); i++) {
			VM->openUpvals[i] = (objUpval*)VM->openUpvals[i]->moveTo;
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
	char* from = oldHeap;
	char* newTop = heap;

	//heapTop points to the top of the old heap, we ONLY update heapTop once we're done with compacting
	while (from < heapTop) {
		obj* curObject = (obj*)from;
		size_t sizeOfObj = getSizeOfObj(curObject);
		char* nextObj = from + sizeOfObj;

		if (forwardAddress(curObject)) {
			char* to = (char*)curObject->moveTo;
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
		moveRaw((char*)to + sizeof(objString), (char*)from + sizeof(objString), ((objString*)from)->length + 1);
		
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
	}
}

void GC::moveRaw(void* to, void* from, size_t size) {
	memcpy(to, from, size);
}

//call the destructor for the appropriate type
void GC::destructObj(obj* ptr) {
	switch (ptr->type) {
	case OBJ_ARRAY:		destruct((objArray*)ptr); break;
	case OBJ_STRING:	destruct((objString*)ptr); break;
	case OBJ_CLOSURE:	destruct((objClosure*)ptr); break;
	case OBJ_FUNC:		destruct((objFunc*)ptr); break;
	case OBJ_NATIVE:	destruct((objNativeFn*)ptr); break;
	case OBJ_UPVALUE:	destruct((objUpval*)ptr); break;
	}
}