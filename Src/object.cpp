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

static uHash hashString(char* str, uInt length) {
	uHash hash = 14695981039346656037u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)str[i];
		hash *= 1099511628211;
	}
	return hash;
}

objString::objString(char* _str, uInt _length, uHash _hash) {
	type = OBJ_STRING;
	length = _length;
	hash = _hash;
	str = _str;
	moveTo = nullptr;
	global::internedStrings.set(this, NIL_VAL());
}

bool objString::compare(char* toCompare, uInt _length) {
	if (length != _length) return false;
	if (str == nullptr) return false;
	for (int i = 0; i < _length; i++) {
		if (toCompare[i] != str[i]) return false;
	}
	return true;
}

//assumes the string hasn't been heap allocated
objString* copyString(char* str, uInt length) {
	uHash hash = hashString(str, length);
	objString* interned = findInternedString(&global::internedStrings, str, length, hash);
	if (interned != nullptr) return interned;
	//we do this because on the heap a string obj is always followed by the char pointer for the string
	//first, alloc enough memory for both the header and the string(length + 1 for null terminator)
	char* temp = (char*)gc.allocRaw(sizeof(objString) + length + 1, true);
	//first we init the header, the char* pointer we pass is where the string will be stored(just past the header)
	objString* onHeap = new(temp) objString(temp + sizeof(objString), length, hash);
	//copy the string contents
	memcpy(temp + sizeof(objString), str, length + 1);
	return onHeap;
}

//works on the same basis as copyString, but it assumed that "str" has been heap allocated, so it frees it
objString* takeString(char* str, uInt length) {
	uHash hash = hashString(str, length);
	objString* interned = findInternedString(&global::internedStrings, str, length, hash);
	if (interned != nullptr) return interned;
	//we do this because on the heap a string obj is always followed by the char pointer for the string
	char* temp = (char*)gc.allocRaw(sizeof(objString) + length + 1, true);
	objString* onHeap = new(temp) objString(temp + sizeof(objString), length, hash);
	memcpy(temp + sizeof(objString), str, length + 1);
	delete str;
	return onHeap;
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

//this is how we get the pointer operator new wants
void* __allocObj(size_t size) {
	return gc.allocRaw(size, true);
}