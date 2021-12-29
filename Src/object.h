#ifndef __IFS_OBJECT
#define __IFS_OBJECT
#include "common.h"
#include "value.h"

enum ObjType {
	OBJ_STRING,
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


#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define IS_STRING(value)       isObjType(value, OBJ_STRING)


#define AS_STRING(value)       ((objString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((objString*)AS_OBJ(value))->str)//gets raw string

objString* copyString(string& str);
objString* takeString(string& str);

void printObject(Value value);
void freeObject(obj* object);

#endif // !__IFS_OBJECT
