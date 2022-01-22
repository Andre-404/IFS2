#ifndef __IFS_OBJECT
#define __IFS_OBJECT
#include "common.h"
#include "value.h"
#include "chunk.h"

enum ObjType {
	OBJ_STRING,
	OBJ_FUNC,
	OBJ_NATIVE,
	OBJ_ARRAY,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
};

//pointer to a native function
typedef Value(*NativeFn)(int argCount, Value* args);
//used for strings
typedef unsigned int uInt;
typedef unsigned long long uHash;


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


class objVectorBase : public obj {
public:
	void* arr;
	int sizeOfType;
	int length;
};


template<typename T>
class objVector : public objVectorBase {
public:
	T* arr;
	objVector(T* _arr, int _length) {
		arr = _arr;
		sizeOfType = sizeof(T);
		length = _length;
	}
};


//Actualy objects

class objArray : public obj {
public:
	vector<Value> values;
	objArray(vector<Value> vals);
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


#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNC)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_ARRAY(value)		   isObjType(value, OBJ_ARRAY)
#define IS_CLOSURE(value)	   isObjType(value, OBJ_CLOSURE)

#define AS_STRING(value)       ((objString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((objString*)AS_OBJ(value))->str)//gets raw string
#define AS_FUNCTION(value)     ((objFunc*)AS_OBJ(value))
#define AS_NATIVE(value) 	   (((objNativeFn*)AS_OBJ(value)))
#define AS_ARRAY(value)		   (((objArray*)AS_OBJ(value)))
#define AS_CLOSURE(value)	   (((objClosure*)AS_OBJ(value)))

objString* copyString(char* str, uInt length);
objString* takeString(char* str, uInt length);

template<typename T>
objVector<T>* createVector(uInt length) {
	char* ptr = (char*)__allocObj(sizeof(objVector<T>) + (sizeof(T) * length));
	objVector<T>* vec = new(ptr) objVector<T>(nullptr);
	vec->arr = new(ptr + sizeof(objVector<T>)) T[length];
	return vec;
}

void printObject(Value value);
void freeObject(obj* object);


#endif // !__IFS_OBJECT
