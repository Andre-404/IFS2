#ifndef __IFS_MEMORY
#define __IFS_MEMORY

#include "common.h"
#include "VM.h"

#define HEAP_START_SIZE (1024*1024*128)

class GC {
public:
	void collect(obj* onStack);
	GC();
	void ready(vm* VM);
	bool isReady;
	//have to do this here because templates.....
	template<typename T>
	T* allocObj(T val) {
		collect(&val);
		T* temp = new(heapTop) T(val);
		if ((heapTop + sizeof(T)) - heap >= HEAP_START_SIZE) {
			std::cout << "Overflow" << "\n";
			exit(64);
		}
		heapTop += sizeof(T);
		allocated += sizeof(T);
		if (temp->type == OBJ_STRING) global::internedStrings.set((objString*)temp, NIL_VAL());
		return temp;
	}
	void clear();
private:
	long sinceLastClear;
	long threshold;
	vm* VM;
	char* heap;
	char* heapTop;
	size_t allocated;

	bool collecting;
	std::vector<obj*> stack;

	void mark();
	void markObj(obj* ptr);
	void markVal(Value& val);
	void markTable(hashTable& table);
	void markRoots();
	void traceObj(obj* ptr);

	void computeAddress();

	void updatePtrs(obj* onStack);
	void updateRootPtrs();
	void updateHeapPtrs();

	void compact();

	void moveObj(void* to, obj* from);
	void deleteObj(obj* ptr);

	template<typename T> void moveData(void* to, T* from) {
		T temp = *from;
		from->~T();
		new(to) T(temp);
	}
	template<typename T> void destruct(T* ptr) {
		ptr->~T();
	}

};

#define GET_OBJ(type, val) (*((type*)val))

#endif // !__IFS_MEMORY
