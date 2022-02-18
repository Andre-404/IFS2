#pragma once

#include "common.h"


void* __allocObj(size_t size);

//The header part of every heap allocated object
class managed {
public:
	managed* moveTo;
	//methods that are supposed to be overridden by every subclass, they are used by the gc
	virtual void move(byte* to) = 0;
	virtual size_t getSize() = 0;
	virtual void updatePtrs() = 0;
	virtual void trace(std::vector<managed*>& stack) = 0;

	virtual ~managed() {}

	//this reroutes the new operator to take memory which the GC gives out
	//this is useful because in case a collection, it happens before the current object is even initalized
	void* operator new(size_t size) {
		return __allocObj(size);
	}
	void* operator new(size_t size, void* to) {
		return to;
	}
	void operator delete(void* memoryBlock) {
		//empty
	}
};