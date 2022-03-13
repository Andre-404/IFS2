#include "builtInFunction.h"
#include "namespaces.h"
#include "files.h"
#include "fiber.h"

using namespace global;
//throwing strings for errors that get caught in callValue()
//fiber is passes as a arg to push values, note that pushing will override 'args' field as it's no longer considered part of the stack

#pragma region Arrays

void nativeArrayCreate(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected number for size argument.";
	double size = AS_NUMBER(*args);

	objArray* arr = new objArray(size);
	transferValue(fiber, OBJ_VAL(arr));
}
void nativeArrayCopy(objFiber* fiber, int argCount, Value* args) {
	if(!IS_ARRAY(*args)) throw "Expected array for array argument.";
	objArray* dstArr = new objArray();
	objArray* srcArr = AS_ARRAY(*args);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	dstArr->numOfHeapPtr = srcArr->numOfHeapPtr;

	//dstArray is not a cached ptr
	gc.cachePtr(dstArr);
	dstArr->values.addAll(srcArr->values);
	gc.getCachedPtr();
	transferValue(fiber, OBJ_VAL(dstArr));
}
void nativeArrayResize(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for size argument.";

	size_t newSize = AS_NUMBER(*(args + 1));
	size_t oldSize = AS_ARRAY(*args)->values.count();
	objArray* arr = AS_ARRAY(*args);

	arr->values.resize(newSize);
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	arr->numOfHeapPtr = 0;
	int end = (oldSize > newSize ? arr->values.count() : oldSize);
	for (int i = 0; i < end; i++) if (IS_OBJ(arr->values[i])) arr->numOfHeapPtr++;

	transferValue(fiber, NIL_VAL());
}

void nativeArrayPush(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (valuesEqual(*args, *(args + 1))) throw "Can't push a reference to self";
	objArray* arr = AS_ARRAY(*args);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 1))) arr->numOfHeapPtr++;
	arr->values.push(*(args + 1));
	
	transferValue(fiber, NIL_VAL());
}
void nativeArrayPop(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	gcVector<Value>& vals = AS_ARRAY(*args)->values;
	if (vals.count() == 0) throw "Can't pop from empty array.";

	Value val = vals.pop();
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) AS_ARRAY(*args)->numOfHeapPtr--;

	transferValue(fiber, val);
}

void nativeArrayInsert(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 2))) arr->numOfHeapPtr++;
	arr->values.insert(*(args + 2), index);

	transferValue(fiber, NIL_VAL());
}

void nativeArrayDelete(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw "Expected array for array argument.";
	if (!IS_NUMBER(*(args + 1))) throw "Expected number for position argument.";
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));
	Value val = arr->values.removeAt(index);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) arr->numOfHeapPtr--;
	transferValue(fiber, val);
}

void nativeArrayLength(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw "Expected array for array argument.";
	objArray* arr = AS_ARRAY(*args);
	transferValue(fiber, NUMBER_VAL(AS_ARRAY(*args)->values.count()));
}

#pragma endregion

#pragma region IO
void nativeFileRead(objFiber* fiber, int argCount, Value* args) {
	if (!IS_STRING(*args)) throw "Expected path name.";
	string fileData = readFile(AS_STRING(*args)->str);
	objString* str = copyString(fileData.c_str(), fileData.length());
	transferValue(fiber, OBJ_VAL(str));
}

void nativeInput(objFiber* fiber, int argCount, Value* args) {
	string input;
	std::cin >> input;
	objString* str = copyString(input.c_str(), input.length());
	transferValue(fiber, OBJ_VAL(str));
}


#pragma endregion


void nativeClock(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, NUMBER_VAL((double)clock() / CLOCKS_PER_SEC));
}

void nativeFloor(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected a number.";
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(floor(d)));
}

void nativeCeil(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected a number.";
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(ceil(d)));
}

void nativeRandomRange(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args) || !IS_NUMBER(*(args + 1))) throw "Expected a number.";
	transferValue(fiber, NUMBER_VAL(std::uniform_int_distribution<int>(AS_NUMBER(*args), AS_NUMBER(*(args + 1)))(rng)));
}

void nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw "Expected a number.";
	double seed = AS_NUMBER(*args);
	if (seed == -1) rng = std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count());
	else rng = std::mt19937(seed);
	transferValue(fiber, NIL_VAL());
}


void nativeToString(objFiber* fiber, int argCount, Value* args) {
	//Either creates a new objString and pushes it onto the fiber stack,
	//or creates a new callframe if *args is a instance whose class defines toString
	if(!pushValToStr(fiber, *args)) throw "";
}
