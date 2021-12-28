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
	string* str;
	objString(string* _str);
	~objString();
};


#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define IS_STRING(value)       isObjType(value, OBJ_STRING)


#define AS_STRING(value)       ((objString*)AS_OBJ(value))
#define AS_CSTRING(value)      (*((objString*)AS_OBJ(value))->str)//gets raw string

objString* copyString(string& str);//takes a string value a heap allocates it
objString* takeString(string* str);//assumes the string has already been heap allocated

void printObject(Value value);
void freeObject(obj* object);

#endif // !__IFS_OBJECT
