#include "gcVector.h"
#include "memory.h"
#include "namespaces.h"


arrHeader::arrHeader(uInt _capacity, uInt _sizeofType) {
	count = 0;
	capacity = _capacity;
	sizeofType = _sizeofType;
	arr = (reinterpret_cast<byte*>(this)) + sizeof(arrHeader);
	moveTo = nullptr;
}

void* arrHeader::operator new(size_t size, size_t arrSize) {
	return global::gc.allocRaw(size + arrSize, true);
}