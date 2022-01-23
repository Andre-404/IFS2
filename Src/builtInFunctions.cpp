#include "builtInFunction.h"
#include "namespaces.h"

using namespace global;
//throwing strings for errors that get caught in callValue()

#pragma region Arrays

Value nativeArrayCreate(int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected number for size argument.";
	double size = AS_NUMBER(*args);
	objArrayHeader* vals = createArr(size);
	objArray* arr = new objArray(vals);
	return OBJ_VAL(arr);
}
Value nativeArrayCopy(int argCount, Value* args) {
	if(!IS_ARRAY(*args))throw "Expected array for array argument.";
	objArrayHeader* vals = createArr(AS_ARRAY(*args)->values->capacity);
	memmove(vals->arr, AS_ARRAY(*args)->values->arr, AS_ARRAY(*args)->values->count * sizeof(Value));
	return OBJ_VAL(new objArray(vals));
}
Value nativeArrayResize(int argCount, Value* args) {
	/*if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for size argument.";

	vector<Value>& vals = AS_ARRAY(*args)->values;
	double size = AS_NUMBER(*(args + 1));
	vals.resize(size, NIL_VAL());*/
	return NIL_VAL();
}

Value nativeArrayPush(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (valuesEqual(*args, *(args + 1))) throw "Can't push a reference to self";
	size_t count = AS_ARRAY(*args)->values->count;
	size_t capacity = AS_ARRAY(*args)->values->capacity;
	if ((count + 1) >= capacity) {
		objArrayHeader* temp = createArr(capacity * 2);
		memcpy(temp->arr, AS_ARRAY(*args)->values->arr, temp->capacity * sizeof(Value));
		temp->count = count;
		AS_ARRAY(*args)->values = temp;
	}
	objArray* arr = AS_ARRAY(*args);
	AS_ARRAY(*args)->values->arr[count] = *(args + 1);
	AS_ARRAY(*args)->values->count++;
	
	return NIL_VAL();
}
Value nativeArrayPop(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	objArrayHeader* vals = AS_ARRAY(*args)->values;
	Value popped = vals->arr[vals->count -1];
	vals->count--;
	return popped;
}
Value nativeArrayInsert(int argCount, Value* args) {
	/*if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";
	
	vector<Value>& vals = AS_ARRAY(*args)->values;
	auto it = vals.begin();
	int i = AS_NUMBER(*(args + 1));
	if (i < 0 || i > vals.size() - 1) throw "Index out of range";
	vals.insert(it + i, *(args + 2));
	*/
	return NIL_VAL();
}
Value nativeArrayDelete(int argCount, Value* args) {
	/*if (!IS_ARRAY(*args)) throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";

	vector<Value>& vals = AS_ARRAY(*args)->values;
	auto it = vals.begin();
	int i = AS_NUMBER(*(args + 1));
	if (i < 0 || i > vals.size() - 1) throw "Index out of range";
	vals.erase(it + i);
	*/
	return NIL_VAL();
}

Value nativeArrayLength(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";

	return NUMBER_VAL(AS_ARRAY(*args)->values->count);
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
