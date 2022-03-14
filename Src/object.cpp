#include "object.h"
#include "namespaces.h"
#include <immintrin.h>

using namespace global;

string funcToStr(objFunc* func) {
	if (!func->name) {
		return "<script>";
	}
	return "<fn" + string(func->name->str) + ">";
}

string objectToStr(Value object) {
	switch (OBJ_TYPE(object)) {
	case OBJ_STRING:
		return string(AS_CSTRING(object));
	case OBJ_FUNC:
		return funcToStr(AS_FUNCTION(object));
	case OBJ_NATIVE:
		return "<native fn>";
	case OBJ_CLOSURE:
		return funcToStr(AS_CLOSURE(object)->func);
	case OBJ_UPVALUE:
		return "<upvalue>";
	case OBJ_ARRAY: {
		return "<array>";
	}
	case OBJ_CLASS: 
		return "<" + string(AS_CLASS(object)->name->str) + ">";
	case OBJ_INSTANCE: {
		objInstance* inst = AS_INSTANCE(object);
		if (inst->klass != nullptr) return "<" + string(AS_INSTANCE(object)->klass->name->str) + " instance>";
		else {
			return "<struct>";
		}
	}
	case OBJ_BOUND_METHOD: 
		return "<" + funcToStr(AS_BOUND_METHOD(object)->method->func) + ">";
	case OBJ_MODULE: {
		objModule* mod = AS_MODULE(object);
		return "<module " + string(mod->name->str) + ">";
	}
	case OBJ_FIBER: 
		return "<fiber>";
	}
}

static uInt64 hashString(char* str, uInt length) {
	uInt64 hash = 14695981039346656037u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)str[i];
		hash *= 1099511628211;
	}
	return hash;
}

bool objString::compare(char* toCompare, uInt _length) {
	if (length != _length) return false;
	if (str == nullptr) return false;
	for (int i = 0; i < _length; i++) {
		if (toCompare[i] != str[i]) return false;
	}
	return true;
}

bool objString::compare(string& str) {
	return compare((char*)str.c_str(), str.size());
}

//assumes the string hasn't been heap allocated
objString* copyString(char* str, uInt length) {
	uInt64 hash = hashString(str, length);
	objString* interned = findInternedString(&global::internedStrings, str, length, hash);
	if (interned != nullptr) return interned;
	//we do this because on the heap a string obj is always followed by the char pointer for the string
	//first, alloc enough memory for both the header and the string(length + 1 for null terminator)
	//if the string is over 100 characters long we allocate it on the large object heap
	char* temp = (char*)gc.allocRaw(sizeof(objString) + length + 1, false);
	//copy the string contents
	memcpy(temp + sizeof(objString), str, length + 1);
	//first we init the header, the char* pointer we pass is where the string will be stored(just past the header)
	objString* onHeap = new(temp) objString(temp + sizeof(objString), length, hash);
	return onHeap;
}

objString* copyString(const char* str, uInt length) {
	return copyString((char*)str, length);
}

objString* copyString(string str) {
	return copyString(str.c_str(), str.size());
}

//works on the same basis as copyString, but it assumed that "str" has been heap allocated, so it frees it
objString* takeString(char* str, uInt length) {
	uInt64 hash = hashString(str, length);
	objString* interned = findInternedString(&global::internedStrings, str, length, hash);
	if (interned != nullptr) return interned;
	//we do this because on the heap a string obj is always followed by the char pointer for the string
	//if the length is over 100 characters we allocate it on the large object heap
	char* temp = (char*)gc.allocRaw(sizeof(objString) + length + 1, false);
	objString* onHeap = new(temp) objString(temp + sizeof(objString), length, hash);
	memcpy(temp + sizeof(objString), str, length + 1);
	delete str;
	return onHeap;
}

#pragma region Helpers
//a way to move objects in memory while taking care of things like STL containers
template<typename T>
void moveData(void* to, T* from) {
	T temp = *from;
	from->~T();
	T* test = new(to) T(temp);
}

void setMarked(obj* ptr) {
	ptr->moveTo = ptr;//a non null pointer means this is a marked object
}

void markObj(std::vector<managed*>& stack, obj* ptr) {
	if (!ptr) return;
	if (ptr->moveTo != nullptr) return;
	//there is no reason to push native function and strings to the stack, we can't trace any further nodes from them
	if (ptr->type == OBJ_NATIVE || ptr->type == OBJ_STRING) {
		setMarked(ptr);
		return;
	}
	stack.push_back(ptr);

}

void markVal(std::vector<managed*>& stack, Value& val) {
	if (val.type == VAL_OBJ) markObj(stack, AS_OBJ(val));
}

void markTable(std::vector<managed*>& stack, hashTable& table) {
	for (int i = 0; i < table.capacity; i++) {
		entry* _entry = &table.entries[i];
		if (_entry->key && _entry->key != TOMBSTONE) {
			markObj(stack, _entry->key);
			markVal(stack, _entry->val);
		}
	}
	//marks the internal array header
	table.entries.mark();
}
#pragma endregion


#pragma region objString
objString::objString(char* _str, uInt _length, uInt64 _hash) {
	length = _length;
	hash = _hash;
	str = _str;
	moveTo = nullptr;
	type = OBJ_STRING;
	global::internedStrings.set(this, NIL_VAL());
}


void objString::updatePtrs() {
	str = ((byte*)moveTo) + sizeof(objString);
}

void objString::move(byte* to) {
	uInt l = length;
	str = to + sizeof(objString);
	memmove(to, ((byte*)this), sizeof(objString) + l + 1);
}
#pragma endregion

#pragma region objFunc
objFunc::objFunc() {
	arity = 0;
	upvalueCount = 0;
	type = OBJ_FUNC;
	moveTo = nullptr;
	name = nullptr;
	std::cout << "Size: " << sizeof(objFunc)<<"\n";
}

void objFunc::move(byte* to) {
	memmove(to, this, sizeof(objFunc));
}

void objFunc::trace(std::vector<managed*>& stack) {
	if (name != nullptr) markObj(stack, name);
	for (int i = 0; i < body.constants.count(); i++) {
		Value& val = body.constants[i];
		markVal(stack, val);
	}
	body.constants.mark();
	body.code.mark();
	for (int i = 0; i < body.switchTables.size(); i++) {
		body.switchTables[i].arr.mark();
	}
}

void objFunc::updatePtrs() {
	if (name != nullptr) name = (objString*)name->moveTo;
	int size = body.constants.count();
	for (int i = 0; i < size; i++) {
		updateVal(&body.constants[i]);
	}
	body.constants.update();
	body.code.update();
	for (int i = 0; i < body.switchTables.size(); i++) {
		body.switchTables[i].arr.update();
	}
}
#pragma endregion


#pragma region objNativeFn
objNativeFn::objNativeFn(NativeFn _func, int _arity) {
	func = _func;
	arity = _arity;
	type = OBJ_NATIVE;
	moveTo = nullptr;
}

void objNativeFn::move(byte* to) {
	memmove(to, this, sizeof(objNativeFn));
}
#pragma endregion


#pragma region objClosure
objClosure::objClosure(objFunc* _func) {
	func = _func;
	upvals.resize(func->upvalueCount, nullptr);
	type = OBJ_CLOSURE;
	moveTo = nullptr;
}

void objClosure::move(byte* to) {
	moveData(to, this);
}

void objClosure::updatePtrs() {
	func = (objFunc*)func->moveTo;
	long size = upvals.size();
	for (int i = 0; i < size; i++) {
		if (upvals[i] != nullptr) upvals[i] = (objUpval*)upvals[i]->moveTo;
	}
}

void objClosure::trace(std::vector<managed*>& stack) {
	markObj(stack, func);
	for (objUpval* upval : upvals) {
		markObj(stack, upval);
	}
}
#pragma endregion

#pragma region objUpval
objUpval::objUpval(Value* slot) {
	location = slot;//this will have to be updated when moving objUpval
	closed = NIL_VAL();
	isOpen = true;
	type = OBJ_UPVALUE;
	moveTo = nullptr;
}

void objUpval::move(byte* to) {
	memmove(to, this, sizeof(objUpval));
}

void objUpval::trace(std::vector<managed*>& stack) {
	if(!isOpen) markVal(stack, closed);
}

void objUpval::updatePtrs() {
	updateVal(&closed);
}
#pragma endregion

#pragma region objArray
objArray::objArray() {
	type = OBJ_ARRAY;
	moveTo = nullptr;
	numOfHeapPtr = 0;
	values = gcVector<Value>();
}

objArray::objArray(size_t size) {
	values = gcVector<Value>(size);
	type = OBJ_ARRAY;
	moveTo = nullptr;
	numOfHeapPtr = 0;
}

void objArray::move(byte* to) {
	memmove(to, this, sizeof(objArray));
}

void objArray::trace(std::vector<managed*>& stack) {
	if (numOfHeapPtr > 0) {
		for (int i = 0; i < values.count(); i++) {
			markVal(stack, values[i]);
		}
	}
	values.mark();
}

void objArray::updatePtrs() {
	if (numOfHeapPtr > 0) {
		for (int i = 0; i < values.count(); i++) {
			updateVal(&values[i]);
		}
	}
	values.update();
}
#pragma endregion

#pragma region objClass
objClass::objClass(objString* _name) {
	name = _name;
	type = OBJ_CLASS;
	moveTo = nullptr;
}

void objClass::move(byte* to) {
	memmove(to, this, sizeof(objClass));
}

void objClass::trace(std::vector<managed*>& stack) {
	name->moveTo = name;
	markTable(stack, methods);
}

void objClass::updatePtrs() {
	name = (objString*)name->moveTo;
	updateTable(&methods);
}
#pragma endregion

#pragma region objInstance
objInstance::objInstance(objClass* _klass) {
	klass = _klass;
	moveTo = nullptr;
	type = OBJ_INSTANCE;
	table = hashTable();
}

void objInstance::move(byte* to) {
	memmove(to, this, sizeof(objInstance));
}

void objInstance::trace(std::vector<managed*>& stack) {
	markTable(stack, table);
	if (klass != nullptr) markObj(stack, klass);
}

void objInstance::updatePtrs() {
	if (klass != nullptr) klass = (objClass*)klass->moveTo;
	updateTable(&table);
}
#pragma endregion

#pragma region objBoundMethod
objBoundMethod::objBoundMethod(Value _receiver, objClosure* _method) {
	receiver = _receiver;
	method = _method;
	type = OBJ_BOUND_METHOD;
	moveTo = nullptr;
}

void objBoundMethod::move(byte* to) {
	memmove(to, this, sizeof(objBoundMethod));
}

void objBoundMethod::trace(std::vector<managed*>& stack) {
	markVal(stack, receiver);
	markObj(stack, method);
}

void objBoundMethod::updatePtrs() {
	method = (objClosure*)method->moveTo;
	updateVal(&receiver);
}
#pragma endregion

#pragma region objModule
objModule::objModule(objString* _name) {
	name = _name;
	type = OBJ_MODULE;
	moveTo = nullptr;
}

void objModule::move(byte* to) {
	memmove(to, this, sizeof(objModule));
}

void objModule::updatePtrs() {
	updateTable(&vars);
	name = reinterpret_cast<objString*>(name->moveTo);
}

void objModule::trace(std::vector<managed*>& stack) {
	name->moveTo = name;
	markTable(stack, vars);
}

#pragma endregion
