#include "value.h"

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUM:    return AS_NUMBER(a) == AS_NUMBER(b);
    default:         return false; // Unreachable.
    }
}

Value BOOL_VAL(bool boolean) {
    Value val;
    val.type = VAL_BOOL;
    val.as.boolean = boolean;
    return val;
}

Value NIL_VAL() {
    Value val;
    val.type = VAL_NIL;
    val.as.num = 0;
    return val;
}
Value NUMBER_VAL(double number) {
    Value val;
    val.type = VAL_NUM;
    val.as.num = number;
    return val;
}