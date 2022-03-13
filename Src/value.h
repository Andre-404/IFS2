#pragma once

#include "common.h"

class obj;

enum valueType {
	VAL_NUM,
	VAL_BOOL,
	VAL_NIL,
	VAL_OBJ
};

union valUnion {
	double num;
	bool boolean;
	obj* object;
};

struct Value {
	valueType type;
	valUnion as;
	Value() {
		type = VAL_NIL;
		as.num = 0;
	}
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
void printValue(Value val);

string valueTypeToStr(Value val);
string valueToStr(Value val);

//Using epsilon value because of floating point precision
#define FLOAT_EQ(x,v) (fabs(x - v) <= DBL_EPSILON)

