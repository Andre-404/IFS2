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
};
/*
If a object has some heap allocated stuff inside of it, delete it inside the destructor, not inside freeObject
*/
class obj{
public:
 ObjType type;
 obj* next;
};

class objString : public obj {
public:
	string str;
	unsigned long long hash;
	objString(string _str, unsigned long long _hash);
	~objString();
};

class objFunc : public obj {
public:
	chunk body;
	//GC worries about the string
	objString* name;
	int arity;
	objFunc();
};


typedef Value (*NativeFn)(int argCount, Value* args);

class objNativeFn : public obj {
public:
	NativeFn func;
	int arity;
	objNativeFn(NativeFn _func, int _arity);
};


class objArray : public obj {
public:
	vector<Value> values;
};

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNC)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_ARRAY(value)		   isObjType(value, OBJ_ARRAY)

#define AS_STRING(value)       ((objString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((objString*)AS_OBJ(value))->str)//gets raw string
#define AS_FUNCTION(value)     ((objFunc*)AS_OBJ(value))
#define AS_NATIVE(value) 	   (((objNativeFn*)AS_OBJ(value)))
#define AS_ARRAY(value)		   (((objArray*)AS_OBJ(value)))

objString* copyString(string& str);
objString* takeString(string& str);

void printObject(Value value);
void freeObject(obj* object);

#endif // !__IFS_OBJECT
