#include "value.h"
#include "object.h"
#include <cmath>

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return false;
    case VAL_NUM: {
        return FLOAT_EQ(AS_NUMBER(a), AS_NUMBER(b));
    }
    case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
    default:         return false; // Unreachable.
    }
}



void printValue(Value value) {
    std::cout << valueToStr(value);
}

string valueTypeToStr(Value val) {
    switch (val.type) {
    case VAL_NIL: return "nil";
    case VAL_NUM: return "number";
    case VAL_OBJ: {
        obj* temp = AS_OBJ(val);
        switch (temp->type) {
        case OBJ_ARRAY: return "array";
        case OBJ_BOUND_METHOD: return "method";
        case OBJ_CLASS: return "class " + string(AS_CLASS(val)->name->str);
        case OBJ_CLOSURE: return "function";
        case OBJ_FIBER: return "fiber";
        case OBJ_FUNC: return "function";
        case OBJ_INSTANCE: return AS_INSTANCE(val)->klass == nullptr ? "struct" : "instance";
        case OBJ_MODULE: return "module";
        case OBJ_NATIVE: return "native function";
        case OBJ_STRING: return "string";
        case OBJ_UPVALUE: return "upvalue";
        }
    }
    }
    return "error, couldn't determine type of value";
}

string valueToStr(Value val) {
    switch (val.type) {
    case VAL_BOOL:
        return AS_BOOL(val) ? "true" : "false";
        break;
    case VAL_NIL: return "nil"; break;
    case VAL_NUM: {
        double num = AS_NUMBER(val);
        int prec = num == (int)num ? 0 : 5;
        return std::to_string(num).substr(0, std::to_string(num).find(".") + prec);
        break;
    }
    case VAL_OBJ: return objectToStr(val); break;
    default:
        std::cout << "Error printing object";
        return "";
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