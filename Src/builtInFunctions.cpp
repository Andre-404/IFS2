#include "builtInFunction.h"
#include "namespaces.h"
#include "files.h"

using namespace global;
//throwing strings for errors that get caught in callValue()

#pragma region Arrays

Value nativeArrayCreate(int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected number for size argument.";
	double size = AS_NUMBER(*args);

	objArray* arr = new objArray(size);
	return OBJ_VAL(arr);
}
Value nativeArrayCopy(int argCount, Value* args) {
	if(!IS_ARRAY(*args)) throw "Expected array for array argument.";
	objArray* dstArr = new objArray(AS_ARRAY(*args)->values.capacity());
	objArray* srcArr = AS_ARRAY(*args);
	dstArr->values.addAll(srcArr->values);
	return OBJ_VAL(dstArr);
}
Value nativeArrayResize(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for size argument.";

	size_t newSize = AS_NUMBER(*(args + 1));
	objArray* arr = AS_ARRAY(*args);

	arr->values.resize(newSize);
	return NIL_VAL();
}

Value nativeArrayPush(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (valuesEqual(*args, *(args + 1))) throw "Can't push a reference to self";
	objArray* arr = AS_ARRAY(*args);

	arr->values.push(*(args + 1));
	
	return NIL_VAL();
}
Value nativeArrayPop(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	gcVector<Value>& vals = AS_ARRAY(*args)->values;
	if (vals.count() == 0) throw "Can't pop from empty array.";
	return vals.pop();
}

Value nativeArrayInsert(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));

	arr->values.insert(*(args + 2), index);

	return NIL_VAL();
}

Value nativeArrayDelete(int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));
	Value val = arr->values.removeAt(index);
	return val;
}

Value nativeArrayLength(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	objArray* arr = AS_ARRAY(*args);
	return NUMBER_VAL(AS_ARRAY(*args)->values.count());
}

#pragma endregion

#pragma region IO
Value nativeFileRead(int argCount, Value* args) {
	if (!IS_STRING(*args)) throw "Expected path name.";
	string fileData = readFile(AS_STRING(*args)->str);
	objString* str = copyString(fileData.c_str(), fileData.length());
	return OBJ_VAL(str);
}

Value nativeInput(int argCount, Value* args) {
	string input;
	std::cin >> input;
	objString* str = copyString(input.c_str(), input.length());
	return OBJ_VAL(str);
}


#pragma endregion


Value nativeClock(int argCount, Value* args) {
	return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value nativeFloor(int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected a number.";
	double d = AS_NUMBER(*args);
	return NUMBER_VAL(floor(d));
}

Value nativeRandomRange(int argCount, Value* args) {
	if (!IS_NUMBER(*args) || !IS_NUMBER(*(args + 1))) throw "Expected a number.";
	return NUMBER_VAL(std::uniform_int_distribution<int>(AS_NUMBER(*args), AS_NUMBER(*(args + 1)))(rng));
}

Value nativeSetRandomSeed(int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected a number.";
	double seed = AS_NUMBER(*args);
	if (seed == -1) rng = std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count());
	else rng = std::mt19937(seed);
	return NIL_VAL();
}
