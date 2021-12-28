#include "value.h"
#include "object.h"

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return false;
    case VAL_NUM:    return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: {
        //look into alternatives for compare
        objString* aString = AS_STRING(a);
        objString* bString = AS_STRING(b);
        return aString->str->compare((*bString->str)) == 0;
    }
    default:         return false; // Unreachable.
    }
}



void printValue(Value value) {
    switch (value.type) {
    case VAL_BOOL:
        std::cout << (AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL: std::cout << "nil"; break;
    case VAL_NUM: std::cout << AS_NUMBER(value); break;
    case VAL_OBJ: printObject(value); break;
    }
}

//creates a value obj on the stack and 
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

Value OBJ_VAL(obj* object) {
    Value val;
    val.type = VAL_OBJ;
    val.as.object = object;
    return val;
}