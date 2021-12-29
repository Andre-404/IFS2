#include "object.h"
#include "namespaces.h"

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

static unsigned long long hashString(string& str) {
	long long hash = 14695981039346656037u;
	int length = str.size();
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)str.at(i);
		hash *= 1099511628211;
	}
	return hash;
}

objString::objString(string _str, unsigned long long _hash) {
	type = OBJ_STRING;
	str = _str;
	hash = _hash;
	next = NULL;//linked list
	global::internedStrings.set(this, NIL_VAL());
}

objString::~objString() {
}

//assumes the string hasn't been heap allocated
objString* copyString(string& str) {
	unsigned long long hash = hashString(str);
	objString* interned = findInternedString(&global::internedStrings, str, hash);
	if (interned != NULL) return interned;
	return new objString(str, hash);
}

//assumes string is heap allocated
objString* takeString(string& str) {
	unsigned long long hash = hashString(str);
	objString* interned = findInternedString(&global::internedStrings, str, hash);
	if (interned != NULL) {
		return interned;
	}
	return new objString(str, hash);
}