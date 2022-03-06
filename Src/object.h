#pragma once

#include "common.h"
#include "value.h"
#include "chunk.h"
#include "hashTable.h"
#include "managed.h"
#include "gcVector.h"


enum objType {
	OBJ_STRING,
	OBJ_FUNC,
	OBJ_NATIVE,
	OBJ_ARRAY,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_CLASS,
	OBJ_INSTANCE,
	OBJ_BOUND_METHOD,
	OBJ_FIBER
};

//pointer to a native function
typedef Value(*NativeFn)(int argCount, Value* args);


class obj : public managed{
public:
	objType type;
};

//Headers
class objString : public obj {
public:
	char* str;
	uInt length;
	uInt64 hash;
	objString(char* _str, uInt _length, uInt64 _hash);
	bool compare(char* toCompare, uInt _length);

	void move(byte* to);
	size_t getSize() { return sizeof(objString) + length + 1; }
	void updatePtrs();
	void trace(std::vector<managed*>& stack) {};
};


//Actual objects
class objArray : public obj {
public:
	gcVector<Value> values;
	uInt numOfHeapPtr;
	objArray();
	objArray(size_t size);

	void move(byte* to);
	size_t getSize() { return sizeof(objArray); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);
};

class objFunc : public obj {
public:
	chunk body;
	//GC worries about the string
	objString* name;
	int arity;
	int upvalueCount;
	objFunc();

	void move(byte* to);
	size_t getSize() { return sizeof(objFunc); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);
};

class objNativeFn : public obj {
public:
	NativeFn func;
	int arity;
	objNativeFn(NativeFn _func, int _arity);

	void move(byte* to);
	size_t getSize() { return sizeof(objNativeFn); }
	void updatePtrs() {};
	void trace(std::vector<managed*>& stack) {};
};

class objUpval;

class objClosure : public obj {
public:
	objFunc* func;
	//should switch to using a gcVector
	vector<objUpval*> upvals;
	objClosure(objFunc* _func);

	void move(byte* to);
	size_t getSize() { return sizeof(objClosure); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);
};
//Value ptr will get updated manually 
class objUpval : public obj {
public:
	Value* location;
	Value closed;
	bool isOpen;
	objUpval(Value* _location);

	void move(byte* to);
	size_t getSize() { return sizeof(objUpval); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);
};

class objClass : public obj {
public:
	objString* name;
	hashTable methods;
	objClass(objString* _name);

	void move(byte* to);
	size_t getSize() { return sizeof(objClass); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);
};

class objBoundMethod : public obj {
public:
	Value receiver;
	objClosure* method;
	objBoundMethod(Value _receiver, objClosure* _method);

	void move(byte* to);
	size_t getSize() { return sizeof(objBoundMethod); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);
};

class objInstance : public obj {
public:
	objClass* klass;
	hashTable table;
	objInstance(objClass* _klass);

	void move(byte* to);
	size_t getSize() { return sizeof(objInstance); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);
};

class objFiber;


#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

static inline bool isObjType(Value value, objType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNC)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_ARRAY(value)		   isObjType(value, OBJ_ARRAY)
#define IS_CLOSURE(value)	   isObjType(value, OBJ_CLOSURE)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define	IS_FIBER(value)		   isObjType(value, OBJ_FIBER)

#define AS_STRING(value)       ((objString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((objString*)AS_OBJ(value))->str)//gets raw string
#define AS_FUNCTION(value)     ((objFunc*)AS_OBJ(value))
#define AS_NATIVE(value) 	   (((objNativeFn*)AS_OBJ(value)))
#define AS_ARRAY(value)		   (((objArray*)AS_OBJ(value)))
#define AS_CLOSURE(value)	   (((objClosure*)AS_OBJ(value)))
#define AS_CLASS(value)        ((objClass*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((objInstance*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((objBoundMethod*)AS_OBJ(value))
#define AS_FIBER(value)		   ((objFiber*)AS_OBJ(value))

objString* copyString(char* str, uInt length);
objString* copyString(const char* str, uInt length);
objString* takeString(char* str, uInt length);

void printObject(Value value);
void freeObject(obj* object);

void setMarked(obj* ptr);
void markObj(std::vector<managed*>& stack, obj* ptr);
void markVal(std::vector<managed*>& stack, Value& val);
void markTable(std::vector<managed*>& stack, hashTable& table);