#ifndef __IFS_OBJECT
#define __IFS_OBJECT
#include "common.h"
#include "value.h"
#include "chunk.h"
#include "hashTable.h"

enum ObjType {
	OBJ_STRING,
	OBJ_FUNC,
	OBJ_NATIVE,
	OBJ_ARRAY,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_ARR_HEADER,
	OBJ_CLASS,
	OBJ_INSTANCE,
	OBJ_BOUND_METHOD,
};

//pointer to a native function
typedef Value(*NativeFn)(int argCount, Value* args);


void* __allocObj(size_t size);

class obj{
public:
obj* moveTo;
ObjType type;
//this reroutes the new operator to take memory which the GC gives out
//this is useful because in case a collection, it happens before the current object is even initalized
void *operator new(size_t size) {
	return __allocObj(size);
}
void* operator new(size_t size, void* to) {
	return to;
}
void operator delete(void* memoryBlock) {
	//empty
}
};

//Headers

class objString : public obj {
public:
	char* str;
	uInt length;
	uHash hash;
	objString(char* _str, uInt _length,uHash _hash);
	bool compare(char* toCompare, uInt _length);
};


class objArrayHeader : public obj {
public:
	Value* arr;
	uInt capacity;
	uInt count;
	objArrayHeader(Value* _arr, uInt _capacity);
};

//Actualy objects

class objArray : public obj {
public:
	objArrayHeader* values;
	objArray(objArrayHeader* vals);
};

class objFunc : public obj {
public:
	chunk body;
	//GC worries about the string
	objString* name;
	int arity;
	int upvalueCount;
	objFunc();
};

class objNativeFn : public obj {
public:
	NativeFn func;
	int arity;
	objNativeFn(NativeFn _func, int _arity);
};

class objUpval;

class objClosure : public obj {
public:
	objFunc* func;
	vector<objUpval*> upvals;
	objClosure(objFunc* _func);
};
//Value ptr will get updated manually 
class objUpval : public obj {
public:
	Value* location;
	Value closed;
	bool isOpen;
	objUpval(Value* _location);
};

class objClass : public obj {
public:
	objString* name;
	hashTable methods;
	objClass(objString* _name);
};

class objBoundMethod : public obj {
public:
	Value receiver;
	objClosure* method;
	objBoundMethod(Value _receiver, objClosure* _method);
};

class objInstance : public obj {
public:
	objClass* klass;
	hashTable table;
	objInstance(objClass* _klass);
};


#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNC)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_ARRAY(value)		   isObjType(value, OBJ_ARRAY)
#define IS_CLOSURE(value)	   isObjType(value, OBJ_CLOSURE)
#define IS_ARR_HEADER(value)   isObjType(value, OBJ_ARR_HEADER)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)

#define AS_STRING(value)       ((objString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((objString*)AS_OBJ(value))->str)//gets raw string
#define AS_FUNCTION(value)     ((objFunc*)AS_OBJ(value))
#define AS_NATIVE(value) 	   (((objNativeFn*)AS_OBJ(value)))
#define AS_ARRAY(value)		   (((objArray*)AS_OBJ(value)))
#define AS_CLOSURE(value)	   (((objClosure*)AS_OBJ(value)))
#define AS_ARRHEADER(value)    (((objArrayHeader*)AS_OBJ(value)))
#define AS_CLASS(value)        ((objClass*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((objInstance*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((objBoundMethod*)AS_OBJ(value))

objString* copyString(char* str, uInt length);
objString* takeString(char* str, uInt length);

objArray* createArr(size_t size = 16);
objArrayHeader* createArrHeader(size_t size = 16);

void printObject(Value value);
void freeObject(obj* object);


#endif // !__IFS_OBJECT
