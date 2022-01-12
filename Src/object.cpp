#include "object.h"
#include "namespaces.h"


void printFunction(objFunc* func) {
	if (func->name == NULL) {
		printf("<script>");
		return;
	}
	std::cout << "<fn " << func->name->str << ">";
}

void printObject(Value value) {
	switch (OBJ_TYPE(value)) {
	case OBJ_STRING:
		std::cout << AS_CSTRING(value);
		break;
	case OBJ_FUNC:
		printFunction(AS_FUNCTION(value));
		break;
	case OBJ_NATIVE:
		std::cout << "<native fn>";
		break;
	case OBJ_ARRAY: {
		std::cout << "[";
		vector<Value> vals = AS_ARRAY(value)->values;
		for (int i = 0; i < vals.size(); i++) {
			printValue(vals[i]);
			if(i != vals.size() - 1) std::cout << ", ";
		}
		std::cout << "]";
	}
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
	case OBJ_FUNC: {
		delete ((objFunc*)object);
		break;
	}
	case OBJ_NATIVE: {
		delete ((objNativeFn*)object);
		break;
	}
	case OBJ_ARRAY: {
		delete ((objArray*)object);
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
	global::internedStrings.set(this, NIL_VAL());
	next = global::objects;
	global::objects = this;
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

objFunc::objFunc() {
	name = NULL;
	arity = 0;
	type = OBJ_FUNC;
	next = global::objects;
	global::objects = this;
}

objNativeFn::objNativeFn(NativeFn _func, int _arity) {
	func = _func;
	arity = _arity;
	type = OBJ_NATIVE;
	next = global::objects;
	global::objects = this;
}

objArray::objArray(vector<Value> vals) {
	values = vals;
	type = OBJ_ARRAY;
	next = global::objects;
	global::objects = this;
}