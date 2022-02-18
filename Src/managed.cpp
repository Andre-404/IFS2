#include "managed.h"
#include "memory.h"
#include "namespaces.h"
//this is how we get the pointer operator new wants
void* __allocObj(size_t size) {
	return global::gc.allocRaw(size, false);
}