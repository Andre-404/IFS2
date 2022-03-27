#include "builtInFunction.h"
#include "namespaces.h"
#include "files.h"
#include "fiber.h"
#include <math.h>

using namespace global;
//throwing strings for errors that get caught in callValue()
//fiber is passed as a arg to push values to, and the return bool is whether the native func has popped it's own args or not

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

void isString(Value val, int argNum) {
	if (!IS_STRING(val)) throw expectedType("Expected string, argument" + std::to_string(argNum) + " is ", val);
}
#pragma endregion

#pragma region Arrays
bool nativeArrayCreate(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double size = AS_NUMBER(*args);

	objArray* arr = new objArray(size);
	fiber->transferValue(OBJ_VAL(arr));
	return true;
}
bool nativeArrayCopy(objFiber* fiber, int argCount, Value* args) {
	if(!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
	objArray* dstArr = new objArray();
	objArray* srcArr = AS_ARRAY(*args);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	dstArr->numOfHeapPtr = srcArr->numOfHeapPtr;

	//dstArray is not a cached ptr
	gc.cachePtr(dstArr);
	dstArr->values.addAll(srcArr->values);
	gc.getCachedPtr();
	fiber->transferValue(OBJ_VAL(dstArr));
	return true;
}
bool nativeArrayResize(objFiber* fiber, int argCount, Value* args) {
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

	fiber->transferValue(NIL_VAL());
	return true;
}
bool nativeArrayFill(objFiber* fiber, int argCount, Value* args) {
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

	fiber->transferValue(NIL_VAL());
	return true;
}

bool nativeArrayPush(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
	objArray* arr = AS_ARRAY(*args);
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 1))) arr->numOfHeapPtr++;
	arr->values.push(*(args + 1));
	global::gc.getCachedPtr();
	fiber->transferValue(NIL_VAL());
	return true;
}
bool nativeArrayPop(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, argument 0 is ", *args);
	gcVector<Value>& vals = AS_ARRAY(*args)->values;
	if (vals.count() == 0) throw string("Can't pop from empty array.");

	Value val = vals.pop();
	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) AS_ARRAY(*args)->numOfHeapPtr--;

	fiber->transferValue(val);
	return true;
}

bool nativeArrayInsert(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));
	arrRangeError(1, index, arr->values.count(), false);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(*(args + 2))) arr->numOfHeapPtr++;
	arr->values.insert(*(args + 2), index);

	fiber->transferValue(NIL_VAL());
	return true;
}

bool nativeArrayDelete(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args)) throw expectedType("Expected a array, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	objArray* arr = AS_ARRAY(*args);
	double index = AS_NUMBER(*(args + 1));
	arrRangeError(1, index, arr->values.count(), true);
	Value val = arr->values.removeAt(index);

	//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
	if (IS_OBJ(val)) arr->numOfHeapPtr--;
	fiber->transferValue(val);
	return true;
}

bool nativeArrayLength(objFiber* fiber, int argCount, Value* args) {
	if (!IS_ARRAY(*args))throw expectedType("Expected a array, argument 0 is ", *args);
	objArray* arr = AS_ARRAY(*args);
	fiber->transferValue(NUMBER_VAL(AS_ARRAY(*args)->values.count()));
	return true;
}

#pragma endregion

#pragma region IO
bool nativeFileRead(objFiber* fiber, int argCount, Value* args) {
	if (!IS_STRING(*args)) throw "Expected path name.";
	string fileData = readFile(AS_STRING(*args)->str);
	objString* str = copyString(fileData.c_str(), fileData.length());
	fiber->transferValue(OBJ_VAL(str));
	return true;
}

bool nativeInput(objFiber* fiber, int argCount, Value* args) {
	string input;
	std::cin >> input;
	objString* str = copyString(input.c_str(), input.length());
	fiber->transferValue(OBJ_VAL(str));
	return true;
}


#pragma endregion

#pragma region Types
bool nativeIsNumber(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_NUMBER(*args)));
	return true;
}
bool nativeIsNil(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_NIL(*args)));
	return true;
}
bool nativeIsBool(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_BOOL(*args)));
	return true;
}
bool nativeIsString(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_STRING(*args)));
	return true;
}
bool nativeIsArray(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_ARRAY(*args)));
	return true;
}
bool nativeIsStruct(objFiber* fiber, int argCount, Value* args) {
	bool isStruct = false;
	if (IS_INSTANCE(*args)) {
		objInstance* inst = AS_INSTANCE(*args);
		if (inst->klass == nullptr) isStruct = true;
	}
	fiber->transferValue(BOOL_VAL(isStruct));
	return true;
}
bool nativeIsInstance(objFiber* fiber, int argCount, Value* args) {
	bool isInstance = false;
	if (IS_INSTANCE(*args)) {
		objInstance* inst = AS_INSTANCE(*args);
		if (inst->klass != nullptr) isInstance = true;
	}
	fiber->transferValue(BOOL_VAL(isInstance));
	return true;
}
bool nativeIsClass(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_CLASS(*args)));
	return true;
}
bool nativeIsFunction(objFiber* fiber, int argCount, Value* args) {
	bool isFunc = false;
	if (IS_FUNCTION(*args) || IS_CLOSURE(*args)) isFunc = true;
	fiber->transferValue(BOOL_VAL(isFunc));
	return true;
}
bool nativeIsMethod(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_BOUND_METHOD(*args)));
	return true;
}
bool nativeIsModule(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_MODULE(*args)));
	return true;
}
bool nativeIsFiber(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(BOOL_VAL(IS_FIBER(*args)));
	return true;
}
#pragma endregion

#pragma region Math
bool nativeFloor(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(floor(d)));
	return true;
}
bool nativeCeil(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(ceil(d)));
	return true;
}
bool nativeRound(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(round(d)));
	return true;
}
bool nativeMax(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	if(argCount == 1) fiber->transferValue(*args);
	int i = 1;
	double curMax = AS_NUMBER(*args);
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, argument "+ std::to_string(i) + "is ", *(args + i));
		if (AS_NUMBER(*(args + i)) > curMax) curMax = AS_NUMBER(*(args + i));
		i++;
	}
	fiber->transferValue(NUMBER_VAL(curMax));
	return true;
}
bool nativeMin(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	if (argCount == 1) fiber->transferValue(*args);
	int i = 1;
	double curMin = AS_NUMBER(*args);
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, argument " + std::to_string(i) + "is ", *(args + 1));
		if (AS_NUMBER(*(args + i)) < curMin) curMin = AS_NUMBER(*(args + i));
		i++;
	}
	fiber->transferValue(NUMBER_VAL(curMin));
	return true;
}
bool nativeMean(objFiber* fiber, int argCount, Value* args) {
	if (argCount == 0) throw "Expected atleast 1 argument, got 0.";
	int i = 0;
	double median = 0;
	while (i < argCount) {
		if (!IS_NUMBER(*(args + i))) throw expectedType("Expected a number, argument " + std::to_string(i) + "is ", *(args + i));
		median += AS_NUMBER(*(args + i));
		i++;
	}
	fiber->transferValue(NUMBER_VAL(median / argCount));
	return true;
}


bool nativeSin(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(sin(d)));
	return true;
}
bool nativeDsin(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(sin((d*pi)/180)));
	return true;
}
bool nativeCos(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(cos(d)));
	return true;
}
bool nativeDcos(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(cos((d * pi) / 180)));
	return true;
}
bool nativeTan(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(tan(d)));
	return true;
}
bool nativeDtan(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(tan((d * pi) / 180)));
	return true;
}


bool nativeLogn(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args))  throw expectedType("Expected a number, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	double d = AS_NUMBER(*args);
	double d2 = AS_NUMBER(*(args + 1));
	fiber->transferValue(NUMBER_VAL(log(d) / log(d2)));
	return true;
}
bool nativeLog2(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(log2(d)));
	return true;
}
bool nativeLog10(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(log10(d)));
	return true;
}
bool nativeLogE(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(log(d)));
	return true;
}


bool nativePow(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args))  throw expectedType("Expected a number, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	double d = AS_NUMBER(*args);
	double d2 = AS_NUMBER(*(args + 1));
	fiber->transferValue(NUMBER_VAL(pow(d, d2)));
	return true;
}
bool nativeSqr(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(sqrt(d)));
	return true;
}
bool nativeSqrt(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double d = AS_NUMBER(*args);
	fiber->transferValue(NUMBER_VAL(sqrt(d)));
	return true;
}
#pragma endregion

#pragma region Strings
bool nativeStringLength(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	fiber->transferValue(NUMBER_VAL(AS_STRING(*args)->length));
	return true;
}

bool nativeStringInsert(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	isString(*(args + 1), 1);
	if (!IS_NUMBER(*(args + 2))) throw expectedType("Expected number for insert pos, argument 2 is", *(args + 2));
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));
	double pos = AS_NUMBER(*(args + 2));
	strRangeError(2, pos, str->length);

	string newStr(str->str);
	newStr.insert(pos, subStr->str);
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}
bool nativeStringDelete(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for start pos, argument 1 is", *(args + 1));
	if (!IS_NUMBER(*(args + 2))) throw expectedType("Expected number for length, argument 2 is", *(args + 2));
	objString* str = AS_STRING(*args);
	double start = AS_NUMBER(*(args + 1));
	double len = AS_NUMBER(*(args + 2));
	strRangeError(1, start, str->length);
	strRangeError(2, start + len, str->length);

	string newStr(str->str);
	newStr.erase(start, len);
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}
bool nativeStringSubstr(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for start pos, argument 1 is", *(args + 1));
	if (!IS_NUMBER(*(args + 2))) throw expectedType("Expected number for length, argument 2 is", *(args + 2));
	objString* str = AS_STRING(*args);
	double start = AS_NUMBER(*(args + 1));
	double len = AS_NUMBER(*(args + 2));
	strRangeError(1, start, str->length);
	strRangeError(2, start + len, str->length);

	string newStr(str->str);
	newStr = newStr.substr(start, len);
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}
bool nativeStringReplace(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	isString(*(args + 1), 1);
	isString(*(args + 2), 2);
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));
	objString* newSubStr = AS_STRING(*(args + 2));

	string newStr(str->str);
	uInt64 pos = newStr.find(subStr->str);
	while (pos != newStr.npos) {
		newStr.replace(pos, subStr->length, newSubStr->str);
		pos = newStr.find(subStr->str);
	}
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}

bool nativeStringCharAt(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	string newStr(str->str);
	newStr = newStr[pos];
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}
bool nativeStringByteAt(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	fiber->transferValue(NUMBER_VAL(string(str->str)[pos]));
	return true;
}

bool nativeStringPos(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	isString(*(args + 1), 1);
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));

	double pos = -1;
	string newStr(str->str);
	pos = newStr.find(subStr->str);
	fiber->transferValue(NUMBER_VAL(pos));
	return true;
}
bool nativeStringLastPos(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	isString(*(args + 1), 1);
	objString* str = AS_STRING(*args);
	objString* subStr = AS_STRING(*(args + 1));

	double pos = -1;
	string newStr(str->str);
	pos = newStr.rfind(subStr->str);
	fiber->transferValue(NUMBER_VAL(pos));
	return true;
}

bool nativeStringIsUpper(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	char c = *(str->str + (uInt64)pos);
	fiber->transferValue(BOOL_VAL(std::isupper(c)));
	return true;
}
bool nativeStringIsLower(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for insert pos, argument 1 is", *(args + 1));
	objString* str = AS_STRING(*args);
	double pos = AS_NUMBER(*(args + 1));
	strRangeError(1, pos, str->length);

	char c = *(str->str + (uInt64)pos);
	fiber->transferValue(BOOL_VAL(std::islower(c)));
	return true;
}

bool nativeStringLower(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	objString* str = AS_STRING(*args);

	string newStr(str->str);
	for (uInt64 i = 0; i < newStr.length(); i++) {
		if (std::isupper(newStr[i])) newStr[i] = std::tolower(newStr[i]);
	}
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}
bool nativeStringUpper(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	objString* str = AS_STRING(*args);

	string newStr(str->str);
	for (uInt64 i = 0; i < newStr.length(); i++) {
		if (std::islower(newStr[i])) newStr[i] = std::toupper(newStr[i]);
	}
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}

bool nativeStringToDigits(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	objString* str = AS_STRING(*args);


	string newStr = "";
	for (uInt64 i = 0; i < str->length; i++) {
		if ((str->str[i] >= '0' && str->str[i] <= '9') || str->str[i] == '.') newStr += str->str[i];
	}
	fiber->transferValue(OBJ_VAL(copyString(newStr)));
	return true;
}
#pragma endregion

//TODO: finish implementing these
#pragma region Files
bool nativeFileOpen(objFiber* fiber, int argCount, Value* args) {
	isString(*args, 0);
	if(!IS_NUMBER(*(args + 1))) throw expectedType("Expected number for type, argument 1 is", *(args + 1));
	string path(AS_CSTRING(*args));
	objFile* file = new objFile(fileType(AS_NUMBER(*(args + 1))));
	file->file.open(path);
	if (file->file.fail()) throw "Failed to open " + path;
	fiber->transferValue(OBJ_VAL(file));
	return true;
}
bool nativeFileClose(objFiber* fiber, int argCount, Value* args);
bool nativeFileExists(objFiber* fiber, int argCount, Value* args);

bool nativeFileReadString(objFiber* fiber, int argCount, Value* args);
bool nativeFileReadReal(objFiber* fiber, int argCount, Value* args);
bool nativeFileReadLn(objFiber* fiber, int argCount, Value* args);

bool nativeFileWriteString(objFiber* fiber, int argCount, Value* args);
bool nativeFileWriteReal(objFiber* fiber, int argCount, Value* args);
bool nativeFileWriteLn(objFiber* fiber, int argCount, Value* args);
#pragma endregion

bool nativeClock(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(NUMBER_VAL((double)clock() / CLOCKS_PER_SEC));
	return true;
}

bool nativeRandomNumber(objFiber* fiber, int argCount, Value* args) {
	fiber->transferValue(NUMBER_VAL(std::uniform_int_distribution<uInt64>(0, UINT64_MAX)(rng)));
	return true;
}
bool nativeRandomRange(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args))  throw expectedType("Expected a number, argument 0 is ", *args);
	if (!IS_NUMBER(*(args + 1))) throw expectedType("Expected a number, argument 1 is ", *(args + 1));
	fiber->transferValue(NUMBER_VAL(std::uniform_int_distribution<uInt64>(AS_NUMBER(*args), AS_NUMBER(*(args + 1)))(rng)));
	return true;
}
bool nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args) {
	if (!IS_NUMBER(*args)) throw expectedType("Expected a number, argument 0 is ", *args);
	double seed = AS_NUMBER(*args);
	if (seed == -1) rng = std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count());
	else rng = std::mt19937(seed);
	fiber->transferValue(NIL_VAL());
	return true;
}


bool nativeToString(objFiber* fiber, int argCount, Value* args) {
	//Either creates a new objString and pushes it onto the fiber stack,
	//or creates a new callframe if *args is a instance whose class defines toString
	fiber->reduceStack(argCount + 1);
	if(!pushValToStr(fiber, *args)) throw "";
	return false;
}
bool nativeToReal(objFiber* fiber, int argCount, Value* args) {
	Value val = *args;
	if (IS_BOOL(val)) {
		if (AS_BOOL(val)) fiber->transferValue(NUMBER_VAL(1));
		else fiber->transferValue(NUMBER_VAL(0));
	}
	else if (IS_NIL(val)) fiber->transferValue(NUMBER_VAL(0));
	else {
		switch (OBJ_TYPE(*args)) {
		case OBJ_STRING: {
			string str(AS_CSTRING(*args));
			try {
				uInt64 pos;
				double num = std::stod(str, &pos);
				if (pos < str.size())
					throw "Trying to convert invalid string to a number, argument 0 is \"" + str + "\".";
				fiber->transferValue(NUMBER_VAL(num));
			}
			catch (std::invalid_argument) {
				throw "Trying to convert invalid string to a number, argument 0 is \"" + str + "\".";
			}
			break;
		}
		default:
			fiber->transferValue(NUMBER_VAL(((uInt64)AS_OBJ(*args))));
		}
	}
	return true;
}
