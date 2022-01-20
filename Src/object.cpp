#include "object.h"
#include "namespaces.h"

using namespace global;

void printFunction(objFunc* func) {
	if (!func->name) {
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
	case OBJ_CLOSURE:
		printFunction(AS_CLOSURE(value)->func);
		break;
	case OBJ_UPVALUE:
		std::cout << "upvalue";
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
		delete (objString*)object;
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
		objArray* ob = (objArray*)object;
		ob->values.clear();
		std::cout << "Deleted array: " << ob << "\n";
		delete ob;
		break;
	}
	case OBJ_CLOSURE: {
		delete ((objClosure*)object);
		break;
	}
	case OBJ_UPVALUE: {
		delete ((objUpval*)object);
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
	moveTo = nullptr;
}

//assumes the string hasn't been heap allocated
objString* copyString(string& str) {
	unsigned long long hash = hashString(str);
	objString* interned = findInternedString(&global::internedStrings, str, hash);
	if (interned != nullptr) return interned;
	return gc.allocObj(objString(str, hash));
}

//assumes string is heap allocated
objString* takeString(string& str) {
	unsigned long long hash = hashString(str);
	objString* interned = findInternedString(&global::internedStrings, str, hash);
	if (interned != nullptr) {
		return interned;
	}
	return gc.allocObj(objString(str, hash));
}

objFunc::objFunc() {
	arity = 0;
	upvalueCount = 0;
	type = OBJ_FUNC;
	moveTo = nullptr;
	name = nullptr;
}

objNativeFn::objNativeFn(NativeFn _func, int _arity) {
	func = _func;
	arity = _arity;
	type = OBJ_NATIVE;
	moveTo = nullptr;
}

objClosure::objClosure(objFunc* _func) {
	func = _func;
	upvals.resize(func->upvalueCount, NULL);
	type = OBJ_CLOSURE;
	moveTo = nullptr;
}

objUpval::objUpval(Value* slot) {
	location = slot;//this will have to be updated when moving objUpval
	closed = NIL_VAL();
	isOpen = true;
	type = OBJ_UPVALUE;
	moveTo = nullptr;
}

objArray::objArray(vector<Value> vals) {
	values = vals;
	type = OBJ_ARRAY;
	moveTo = nullptr;
}