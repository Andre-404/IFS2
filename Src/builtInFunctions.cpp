#include "builtInFunction.h"
#include "namespaces.h"

using namespace global;
//throwing strings for errors that get caught in callValue()

#pragma region Arrays

Value nativeArrayCreate(int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected number for size argument.";
	vector<Value> vals;
	double size = AS_NUMBER(*args);
	vals.resize(size, NIL_VAL());
	objArray* arr = new objArray(vals);
	return OBJ_VAL(arr);
}
Value nativeArrayCopy(int argCount, Value* args) {
	if(!IS_ARRAY(*args))throw "Expected array for array argument.";
	vector<Value> vals = AS_ARRAY(*args)->values;
	return OBJ_VAL(new objArray(vals));
}
Value nativeArrayResize(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for size argument.";

	vector<Value>& vals = AS_ARRAY(*args)->values;
	double size = AS_NUMBER(*(args + 1));
	vals.resize(size, NIL_VAL());
	return NIL_VAL();
}

Value nativeArrayPush(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	AS_ARRAY(*args)->values.push_back(*(args + 1));
	
	return NIL_VAL();
}
Value nativeArrayPop(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	vector<Value>& vals = AS_ARRAY(*args)->values;
	Value popped = vals[vals.size()-1];
	vals.pop_back();
	return popped;
}
Value nativeArrayInsert(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";
	
	vector<Value>& vals = AS_ARRAY(*args)->values;
	auto it = vals.begin();
	int i = AS_NUMBER(*(args + 1));
	if (i < 0 || i > vals.size() - 1) throw "Index out of range";
	vals.insert(it + i, *(args + 2));

	return NIL_VAL();
}
Value nativeArrayDelete(int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";

	vector<Value>& vals = AS_ARRAY(*args)->values;
	auto it = vals.begin();
	int i = AS_NUMBER(*(args + 1));
	if (i < 0 || i > vals.size() - 1) throw "Index out of range";
	vals.erase(it + i);

	return NIL_VAL();
}

Value nativeArrayLength(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";

	return NUMBER_VAL(AS_ARRAY(*args)->values.size());
}

#pragma endregion

Value clockNative(int argCount, Value* args) {
	return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
