#include "builtInFunction.h"
#include "namespaces.h"

using namespace global;
//throwing strings for errors that get caught in callValue()

#pragma region Arrays

Value nativeArrayCreate(int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected number for size argument.";
	double size = AS_NUMBER(*args);

	objArray* arr = createArr(size);
	return OBJ_VAL(arr);
}
Value nativeArrayCopy(int argCount, Value* args) {
	if(!IS_ARRAY(*args)) throw "Expected array for array argument.";
	objArray* arr = createArr(AS_ARRAY(*args)->values->capacity);
	//we can safely do this since both the actual Value array is of POD type
	memmove(arr->values->arr, AS_ARRAY(*args)->values->arr, AS_ARRAY(*args)->values->count * sizeof(Value));
	return OBJ_VAL(arr);
}
Value nativeArrayResize(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for size argument.";

	size_t count = AS_ARRAY(*args)->values->count;
	size_t capacity = AS_ARRAY(*args)->values->capacity;
	size_t newSize = AS_NUMBER(*(args + 1));

	objArrayHeader* temp = createArrHeader(newSize);
	memcpy(temp->arr, AS_ARRAY(*args)->values->arr, AS_ARRAY(*args)->values->count * sizeof(Value));
	temp->count = newSize;
	AS_ARRAY(*args)->values = temp;
	objArray* arr = AS_ARRAY(*args);
	return NIL_VAL();
}

Value nativeArrayPush(int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (valuesEqual(*args, *(args + 1))) throw "Can't push a reference to self";
	size_t count = AS_ARRAY(*args)->values->count;
	size_t capacity = AS_ARRAY(*args)->values->capacity;
	if ((count + 1) >= capacity) {
		objArrayHeader* temp = createArrHeader(capacity * 2);
		objArray* arr = AS_ARRAY(*args);
		memcpy(temp->arr, arr->values->arr, arr->values->count * sizeof(Value));
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
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";
	
	size_t count = AS_ARRAY(*args)->values->count;
	size_t capacity = AS_ARRAY(*args)->values->capacity;
	if ((count + 1) >= capacity) {
		objArrayHeader* temp = createArrHeader(capacity * 2);
		memcpy(temp->arr, AS_ARRAY(*args)->values->arr, AS_ARRAY(*args)->values->count * sizeof(Value));
		temp->count = count;
		AS_ARRAY(*args)->values = temp;
	}
	objArrayHeader* vals = AS_ARRAY(*args)->values;
	int i = AS_NUMBER(*(args + 1));
	if (i < 0 || i > vals->count) throw "Index out of range";
	if (i != vals->count) {
		memmove(&vals->arr[i + 1], &vals->arr[i], (vals->count - i) * sizeof(Value));
		vals->arr[i] = *(args + 2);
	}
	else {
		vals->arr[i] = *(args + 2);
	}
	vals->count++;
	return NIL_VAL();
}

Value nativeArrayDelete(int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";

	objArrayHeader* vals = AS_ARRAY(*args)->values;
	int i = AS_NUMBER(*(args + 1));
	if (i < 0 || i > vals->count - 1) throw "Index out of range";
	Value val = vals->arr[i];
	if (i != vals->count - 1) {
		memmove(&vals->arr[i], &vals->arr[i + 1], (vals->count - (i + 1))*sizeof(Value));
	}
	vals->count--;
	return val;
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
