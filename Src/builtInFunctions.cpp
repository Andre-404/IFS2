#include "builtInFunction.h"
#include "namespaces.h"
#include "files.h"
#include "fiber.h"
#include <math.h>

using namespace global;
//throwing strings for errors that get caught in callValue()
//fiber is passes as a arg to push values, note that pushing will override 'args' field as it's no longer considered part of the stack

#pragma region Arrays

void nativeArrayCreate(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number for size, got ", *args);
	double size = AS_NUMBER(*args);

	objArray* arr = new objArray(size);
	transferValue(fiber, OBJ_VAL(arr));
}
void nativeArrayCopy(objFiber* fiber, int argCount, Value* args) {
	if(!IS_ARRAY(*args)) throw expectedType("Expected a array, got ", *args);
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
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, got ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, got ", *(args + 1));

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
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, got ", *args);
	objArray* arr = AS_ARRAY(*args);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 1))) arr->numOfHeapPtr++;
	arr->values.push(*(args + 1));
	
	transferValue(fiber, NIL_VAL());
}
void nativeArrayPop(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, got ", *args);
	gcVector<Value>& vals = AS_ARRAY(*args)->values;
	if (vals.count() == 0) throw string("Can't pop from empty array.");

	Value val = vals.pop();
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) AS_ARRAY(*args)->numOfHeapPtr--;

	transferValue(fiber, val);
}

void nativeArrayInsert(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, got ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, got ", *(args + 1));
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 2))) arr->numOfHeapPtr++;
	arr->values.insert(*(args + 2), index);

	transferValue(fiber, NIL_VAL());
}

void nativeArrayDelete(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw expectedType("Expected a array, got ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, got ", *(args + 1));
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));
	Value val = arr->values.removeAt(index);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) arr->numOfHeapPtr--;
	transferValue(fiber, val);
}

void nativeArrayLength(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, got ", *args);
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
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(floor(d)));
}
void nativeCeil(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(ceil(d)));
}
void nativeRound(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(round(d)));
}
void nativeMax(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	if(argCount == 1) transferValue(fiber, *args);
	int i = 1;
	double curMax = AS_NUMBER(*args);
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, got ", *(args + i));
		if (AS_NUMBER(*(args + i)) > curMax) curMax = AS_NUMBER(*(args + i));
		i++;
	}
	transferValue(fiber, NUMBER_VAL(curMax));
}
void nativeMin(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	if (argCount == 1) transferValue(fiber, *args);
	int i = 1;
	double curMin = AS_NUMBER(*args);
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, got ", *(args + 1));
		if (AS_NUMBER(*(args + i)) < curMin) curMin = AS_NUMBER(*(args + i));
		i++;
	}
	transferValue(fiber, NUMBER_VAL(curMin));
}
void nativeMean(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	int i = 0;
	double median = 0;
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, got ", *(args + i));
		median += AS_NUMBER(*(args + i));
		i++;
	}
	transferValue(fiber, NUMBER_VAL(median / argCount));
}


void nativeSin(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sin(d)));
}
void nativeDsin(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sin((d*pi)/180)));
}
void nativeCos(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(cos(d)));
}
void nativeDcos(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(cos((d * pi) / 180)));
}
void nativeTan(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(tan(d)));
}
void nativeDtan(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(tan((d * pi) / 180)));
}


void nativeLogn(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args) || !IS_NUMBER(*(args + 1))) 
		throw "Expected numbers, got " + valueTypeToStr(*args) + ", " + valueTypeToStr(*(args + 1)) + ".";
	double d = AS_NUMBER(*args);
	double d2 = AS_NUMBER(*(args + 1));
	transferValue(fiber, NUMBER_VAL(log(d) / log(d2)));
}
void nativeLog2(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(log2(d)));
}
void nativeLog10(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(log10(d)));
}
void nativeLogE(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(log(d)));
}


void nativePow(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args) || !IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	double d2 = AS_NUMBER(*(args + 1));
	transferValue(fiber, NUMBER_VAL(pow(d, d2)));
}
void nativeSqr(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sqrt(d)));
}
void nativeSqrt(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sqrt(d)));
}


void nativeRandomNumber(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args) || !IS_NUMBER(*(args + 1))) 
		throw "Expected numbers, got " + valueTypeToStr(*args) + ", " + valueTypeToStr(*(args + 1)) + ".";
	transferValue(fiber, NUMBER_VAL(std::uniform_int_distribution<uInt64>(0, UINT64_MAX)(rng)));
}
void nativeRandomRange(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args) || !IS_NUMBER(*(args + 1))) 
		throw "Expected numbers, got " + valueTypeToStr(*args) + ", " + valueTypeToStr(*(args + 1)) + ".";
	transferValue(fiber, NUMBER_VAL(std::uniform_int_distribution<uInt64>(AS_NUMBER(*args), AS_NUMBER(*(args + 1)))(rng)));
}
void nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, got ", *args);
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
void nativeToReal(objFiber* fiber, int argCount, Value* args) {
	Value val = *args;
	if (IS_BOOL(val)) {
		if (AS_BOOL(val)) transferValue(fiber, NUMBER_VAL(1));
		else transferValue(fiber, NUMBER_VAL(0));
	}
	else if (IS_NIL(val)) transferValue(fiber, NUMBER_VAL(0));
	else {
		obj* ptr = AS_OBJ(val);
		double num = (uInt64)ptr;
		transferValue(fiber, NUMBER_VAL(num));
	}
}
