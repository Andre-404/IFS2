#include "object.h"

extern memoryTracker tracker;


void printObject(Value value) {
	switch (OBJ_TYPE(value)) {
	case OBJ_STRING:
		std::cout << AS_CSTRING(value);
		break;
	}
}

//heap allocated objects that are a part of objs are deallocated in it's destructor function
void freeObject(obj* object) {
	switch (object->type) {
	case OBJ_STRING: {
		objString* str = (objString*)object;
		delete str;
		break;
	}
	}
}

objString::objString(string* _str) {
	type = OBJ_STRING;
	str = _str;
	next = NULL;//linked list
}

objString::~objString() {
	delete str;
}

//assumes the string hasn't been heap allocated
objString* copyString(string& str) {
	string* heapStr = new string(str);
	return new objString(heapStr);
}

//assumes string is heap allocated
objString* takeString(string* str) {
	return new objString(str);
}