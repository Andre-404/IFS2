#ifndef __IFS_VALUE
#define __IFS_VALUE

#include "common.h"

class obj;
class objString;

enum valueType {
	VAL_NUM,
	VAL_BOOL,
	VAL_NIL,
	VAL_OBJ
};

struct Value {
	valueType type;
	union {
		double num;
		bool boolean;
		obj* object;
	}as;
};

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUM)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.num)
#define AS_OBJ(value)     ((value).as.object)


Value BOOL_VAL(bool boolean);
Value NIL_VAL();
Value NUMBER_VAL(double number);   
Value OBJ_VAL(obj* object);


bool valuesEqual(Value a, Value b);
void printValue(Value value);

#endif // !__IFS_VALUE
