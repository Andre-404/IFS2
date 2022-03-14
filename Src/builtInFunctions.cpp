#include "builtInFunction.h"
#include "namespaces.h"
#include "files.h"
#include "fiber.h"
#include <math.h>

using namespace global;
//throwing strings for errors that get caught in callValue()
//fiber is passes as a arg to push values, 
//note that pushing will override 'args' field as it's no longer considered part of the stack
//this also means that if some argument is a heap object it will need to be cached

#pragma region Helpers
void arrRangeError(uInt argNum, double index, uInt end, bool strong) {
	if (strong) end--;
	if (index >= 0 && index <= end) return;
	string strIndex = std::to_string(index).substr(0, std::to_string(index).find(".") + 0);
	string strEnd = std::to_string(end).substr(0, std::to_string(end).find(".") + 0);
	throw "Array range is [0, " + strEnd + "],"+ "argument "+ std::to_string(argNum) + " is " + strIndex + ".";
}

void strRangeError(uInt argNum, double index, uInt end) {
	if (index >= 0 && index <= end) return;
	string strIndex = std::to_string(index).substr(0, std::to_string(index).find(".") + 0);
	string strEnd = std::to_string(end).substr(0, std::to_string(end).find(".") + 0);
	throw "String length is " + strEnd + "," + "argument " + std::to_string(argNum) + " is " + strIndex + ".";
}

void isString(Value val) {
	if (!IS_STRING(val)) throw expectedType("Expected string, got ", val);
}
#pragma endregion

#pragma region Arrays
void nativeArrayCreate(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double size = AS_NUMBER(*args);

	objArray* arr = new objArray(size);
	transferValue(fiber, OBJ_VAL(arr));
}
void nativeArrayCopy(objFiber* fiber, int argCount, Value* args) {
	if(!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
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
	if (!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));

	size_t newSize = AS_NUMBER(*(args + 1));
	if (newSize < 0) throw "New size can't be negative.";
	size_t oldSize = AS_ARRAY(*args)->values.count();
	objArray* arr = AS_ARRAY(*args);

	arr->values.resize(newSize);
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	arr->numOfHeapPtr = 0;
	int end = (oldSize > newSize ? arr->values.count() : oldSize);
	for (int i = 0; i < end; i++) if (IS_OBJ(arr->values[i])) arr->numOfHeapPtr++;

	transferValue(fiber, NIL_VAL());
}
void nativeArrayFill(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number for start of range, argument 1 is ", *(args + 1));
	if (!IS_NUMBER(*(args + 2))) throw expectedType("Expected a number for end of range, argument 2 is ", *(args + 2));
	objArray* arr = AS_ARRAY(*args);
	double start = AS_NUMBER(*(args + 1));
	double end = AS_NUMBER(*(args + 2));
	arrRangeError(1, start, arr->values.count(), true);
	arrRangeError(2, end, arr->values.count(), true);
	Value val = *(args + 3);
	bool isObj = IS_OBJ(val);
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	for (uInt64 i = start; i <= end; i++) {
		bool indexIsObj = IS_OBJ(arr->values[i]);
		if(isObj && !indexIsObj) arr->numOfHeapPtr++;
		else if (!isObj && indexIsObj) arr->numOfHeapPtr--;
		arr->values[i] = val;
	}

	transferValue(fiber, NIL_VAL());
}

void nativeArrayPush(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
	objArray* arr = AS_ARRAY(*args);
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 1))) arr->numOfHeapPtr++;
	arr->values.push(*(args + 1));
	global::gc.getCachedPtr();
	transferValue(fiber, NIL_VAL());
}
void nativeArrayPop(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, argument 0 is ", *args);
	gcVector<Value>& vals = AS_ARRAY(*args)->values;
	if (vals.count() == 0) throw string("Can't pop from empty array.");

	Value val = vals.pop();
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) AS_ARRAY(*args)->numOfHeapPtr--;

	transferValue(fiber, val);
}

void nativeArrayInsert(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));
	arrRangeError(1, index, arr->values.count(), false);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 2))) arr->numOfHeapPtr++;
	arr->values.insert(*(args + 2), index);

	transferValue(fiber, NIL_VAL());
}

void nativeArrayDelete(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));
	arrRangeError(1, index, arr->values.count(), true);
	Value val = arr->values.removeAt(index);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) arr->numOfHeapPtr--;
	transferValue(fiber, val);
}

void nativeArrayLength(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, argument 0 is ", *args);
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

#pragma region Types
void nativeIsNumber(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_NUMBER(*args)));
}
void nativeIsNil(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_NIL(*args)));
}
void nativeIsBool(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_BOOL(*args)));
}
void nativeIsString(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_STRING(*args)));
}
void nativeIsArray(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_ARRAY(*args)));
}
void nativeIsStruct(objFiber* fiber, int argCount, Value* args) {
	bool isStruct = false;
	if (IS_INSTANCE(*args)) {
		objInstance* inst = AS_INSTANCE(*args);
		if (inst->klass == nullptr) isStruct = true;
	}
	transferValue(fiber, BOOL_VAL(isStruct));
}
void nativeIsInstance(objFiber* fiber, int argCount, Value* args) {
	bool isInstance = false;
	if (IS_INSTANCE(*args)) {
		objInstance* inst = AS_INSTANCE(*args);
		if (inst->klass != nullptr) isInstance = true;
	}
	transferValue(fiber, BOOL_VAL(isInstance));
}
void nativeIsClass(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_CLASS(*args)));
}
void nativeIsFunction(objFiber* fiber, int argCount, Value* args) {
	bool isFunc = false;
	if (IS_FUNCTION(*args) || IS_CLOSURE(*args)) isFunc = true;
	transferValue(fiber, BOOL_VAL(isFunc));
}
void nativeIsMethod(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_BOUND_METHOD(*args)));
}
void nativeIsModule(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_MODULE(*args)));
}
void nativeIsFiber(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, BOOL_VAL(IS_FIBER(*args)));
}
#pragma endregion


void nativeClock(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, NUMBER_VAL((double)clock() / CLOCKS_PER_SEC));
}

#pragma region Math
void nativeFloor(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(floor(d)));
}
void nativeCeil(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(ceil(d)));
}
void nativeRound(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(round(d)));
}
void nativeMax(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	if(argCount == 1) transferValue(fiber, *args);
	int i = 1;
	double curMax = AS_NUMBER(*args);
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, argument "+ std::to_string(i) + "is ", *(args + i));
		if (AS_NUMBER(*(args + i)) > curMax) curMax = AS_NUMBER(*(args + i));
		i++;
	}
	transferValue(fiber, NUMBER_VAL(curMax));
}
void nativeMin(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	if (argCount == 1) transferValue(fiber, *args);
	int i = 1;
	double curMin = AS_NUMBER(*args);
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, argument " + std::to_string(i) + "is ", *(args + 1));
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
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, argument " + std::to_string(i) + "is ", *(args + i));
		median += AS_NUMBER(*(args + i));
		i++;
	}
	transferValue(fiber, NUMBER_VAL(median / argCount));
}


void nativeSin(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sin(d)));
}
void nativeDsin(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sin((d*pi)/180)));
}
void nativeCos(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(cos(d)));
}
void nativeDcos(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(cos((d * pi) / 180)));
}
void nativeTan(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(tan(d)));
}
void nativeDtan(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(tan((d * pi) / 180)));
}


void nativeLogn(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args))  throw expectedType("Expected a number, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	double d = AS_NUMBER(*args);
	double d2 = AS_NUMBER(*(args + 1));
	transferValue(fiber, NUMBER_VAL(log(d) / log(d2)));
}
void nativeLog2(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(log2(d)));
}
void nativeLog10(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(log10(d)));
}
void nativeLogE(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(log(d)));
}


void nativePow(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args))  throw expectedType("Expected a number, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	double d = AS_NUMBER(*args);
	double d2 = AS_NUMBER(*(args + 1));
	transferValue(fiber, NUMBER_VAL(pow(d, d2)));
}
void nativeSqr(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sqrt(d)));
}
void nativeSqrt(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	transferValue(fiber, NUMBER_VAL(sqrt(d)));
}
#pragma endregion

#pragma region Strings
void nativeStringLength(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	transferValue(fiber, NUMBER_VAL(AS_STRING(*args)->length));
}

void nativeStringInsert(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	isString(*(args + 1));
	if (!IS_NUMBER(*(args + 2))) throw expectedType("Expected number for insert pos, argument 2 is", *(args + 2));
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));
	double pos = AS_NUMBER(*(args + 2));
	strRangeError(2, pos, str->length);

	string newStr(str->str);
	newStr.insert(pos, subStr->str);
	transferValue(fiber, OBJ_VAL(copyString(newStr)));
}
void nativeStringDelete(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for start pos, argument 1 is", *(args + 1));
	if (!IS_NUMBER(*(args + 2))) throw expectedType("Expected number for length, argument 2 is", *(args + 2));
	objString* str = AS_STRING(*args);
	double start = AS_NUMBER(*(args + 1));
	double len = AS_NUMBER(*(args + 2));
	strRangeError(1, start, str->length);
	strRangeError(2, start + len, str->length);

	string newStr(str->str);
	newStr.erase(start, len);
	transferValue(fiber, OBJ_VAL(copyString(newStr)));
}
void nativeStringSubstr(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for start pos, argument 1 is", *(args + 1));
	if (!IS_NUMBER(*(args + 2))) throw expectedType("Expected number for length, argument 2 is", *(args + 2));
	objString* str = AS_STRING(*args);
	double start = AS_NUMBER(*(args + 1));
	double len = AS_NUMBER(*(args + 2));
	strRangeError(1, start, str->length);
	strRangeError(2, start + len, str->length);

	string newStr(str->str);
	newStr = newStr.substr(start, len);
	transferValue(fiber, OBJ_VAL(copyString(newStr)));
}
void nativeStringReplace(objFiber* fiber, int argCount, Value* args) {
	/*isString(*args);
	isString(*(args + 1));
	isString(*(args + 2));
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));
	objString* newSubStr = AS_STRING(*(args + 2));

	string newStr(str->str);
	newStr.insert(pos, string(subStr->str));
	transferValue(fiber, OBJ_VAL(copyString(newStr)));*/
}

void nativeStringCharAt(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	string newStr(str->str);
	newStr = newStr[pos];
	transferValue(fiber, OBJ_VAL(copyString(newStr)));
}
void nativeStringByteAt(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	transferValue(fiber, NUMBER_VAL(string(str->str)[pos]));
}

void nativeStringPos(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	isString(*(args + 1));
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));

	double pos = -1;
	string newStr(str->str);
	pos = newStr.find(subStr->str);
	transferValue(fiber, NUMBER_VAL(pos));
}
void nativeStringLastPos(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	isString(*(args + 1));
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));

	double pos = -1;
	string newStr(str->str);
	pos = newStr.rfind(subStr->str);
	transferValue(fiber, NUMBER_VAL(pos));
}

void nativeStringIsUpper(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	char c = *(str->str + (uInt64)pos);
	transferValue(fiber, BOOL_VAL(std::isupper(c)));
}
void nativeStringIsLower(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	char c = *(str->str + (uInt64)pos);
	transferValue(fiber, BOOL_VAL(std::islower(c)));
}

void nativeStringLower(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	objString* str = AS_STRING(*args);

	string newStr(str->str);
	for (uInt64 i = 0; i < newStr.length(); i++) {
		if (std::isupper(newStr[i])) newStr[i] = std::tolower(newStr[i]);
	}
	transferValue(fiber, OBJ_VAL(copyString(newStr)));
}
void nativeStringUpper(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	objString* str = AS_STRING(*args);

	string newStr(str->str);
	for (uInt64 i = 0; i < newStr.length(); i++) {
		if (std::islower(newStr[i])) newStr[i] = std::toupper(newStr[i]);
	}
	transferValue(fiber, OBJ_VAL(copyString(newStr)));
}

void nativeStringToDigits(objFiber* fiber, int argCount, Value* args) {
	isString(*args);
	objString* str = AS_STRING(*args);


	string newStr = "";
	for (uInt64 i = 0; i < str->length; i++) {
		if ((str->str[i] >= '0' && str->str[i] <= '9') || str->str[i] == '.') newStr += str->str[i];
	}
	transferValue(fiber, OBJ_VAL(copyString(newStr)));
}
#pragma endregion


void nativeRandomNumber(objFiber* fiber, int argCount, Value* args) {
	transferValue(fiber, NUMBER_VAL(std::uniform_int_distribution<uInt64>(0, UINT64_MAX)(rng)));
}
void nativeRandomRange(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args))  throw expectedType("Expected a number, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	transferValue(fiber, NUMBER_VAL(std::uniform_int_distribution<uInt64>(AS_NUMBER(*args), AS_NUMBER(*(args + 1)))(rng)));
}
void nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
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
		switch (OBJ_TYPE(*args)) {
		case OBJ_STRING: {
			string str(AS_CSTRING(*args));
			try {
				uInt64 pos;
				double num = std::stod(str, &pos);
				if (pos < str.size())
					throw "Trying to convert invalid string to a number, argument 0 is " + str + ".";
				transferValue(fiber, NUMBER_VAL(num));
			}
			catch (std::invalid_argument) {
				throw "Trying to convert invalid string to a number, argument 0 is " + str + ".";
			}
			break;
		}
		default:
			transferValue(fiber, NUMBER_VAL(((uInt64)AS_OBJ(*args))));
		}
	}
}
