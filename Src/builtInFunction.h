#pragma once

#include "common.h"
#include "object.h"


#pragma region Arrays

bool nativeArrayCreate(objFiber* fiber, int argCount, Value* args);
bool nativeArrayCopy(objFiber* fiber, int argCount, Value* args);
bool nativeArrayResize(objFiber* fiber, int argCount, Value* args);
bool nativeArrayFill(objFiber* fiber, int argCount, Value* args);

bool nativeArrayPush(objFiber* fiber, int argCount, Value* args);
bool nativeArrayPop(objFiber* fiber, int argCount, Value* args);
bool nativeArrayInsert(objFiber* fiber, int argCount, Value* args);
bool nativeArrayDelete(objFiber* fiber, int argCount, Value* args);

bool nativeArrayLength(objFiber* fiber, int argCount, Value* args);
#pragma endregion

#pragma region Math
bool nativeFloor(objFiber* fiber, int argCount, Value* args);
bool nativeCeil(objFiber* fiber, int argCount, Value* args);
bool nativeRound(objFiber* fiber, int argCount, Value* args);

bool nativeMax(objFiber* fiber, int argCount, Value* args);
bool nativeMin(objFiber* fiber, int argCount, Value* args);
bool nativeMean(objFiber* fiber, int argCount, Value* args);

bool nativeSin(objFiber* fiber, int argCount, Value* args);
bool nativeDsin(objFiber* fiber, int argCount, Value* args);
bool nativeCos(objFiber* fiber, int argCount, Value* args);
bool nativeDcos(objFiber* fiber, int argCount, Value* args);
bool nativeTan(objFiber* fiber, int argCount, Value* args);
bool nativeDtan(objFiber* fiber, int argCount, Value* args);

bool nativeLogn(objFiber* fiber, int argCount, Value* args);
bool nativeLog10(objFiber* fiber, int argCount, Value* args);
bool nativeLog2(objFiber* fiber, int argCount, Value* args);
bool nativeLogE(objFiber* fiber, int argCount, Value* args);

bool nativePow(objFiber* fiber, int argCount, Value* args);
bool nativeSqrt(objFiber* fiber, int argCount, Value* args);
bool nativeSqr(objFiber* fiber, int argCount, Value* args);
#pragma endregion

#pragma region Types
bool nativeIsNumber(objFiber* fiber, int argCount, Value* args);
bool nativeIsNil(objFiber* fiber, int argCount, Value* args);
bool nativeIsBool(objFiber* fiber, int argCount, Value* args);
bool nativeIsString(objFiber* fiber, int argCount, Value* args);
bool nativeIsArray(objFiber* fiber, int argCount, Value* args);
bool nativeIsStruct(objFiber* fiber, int argCount, Value* args);
bool nativeIsInstance(objFiber* fiber, int argCount, Value* args);
bool nativeIsClass(objFiber* fiber, int argCount, Value* args);
bool nativeIsFunction(objFiber* fiber, int argCount, Value* args);
bool nativeIsMethod(objFiber* fiber, int argCount, Value* args);
bool nativeIsModule(objFiber* fiber, int argCount, Value* args);
bool nativeIsFiber(objFiber* fiber, int argCount, Value* args);
#pragma endregion

#pragma region Strings
bool nativeStringLength(objFiber* fiber, int argCount, Value* args);

bool nativeStringInsert(objFiber* fiber, int argCount, Value* args);
bool nativeStringDelete(objFiber* fiber, int argCount, Value* args);
bool nativeStringSubstr(objFiber* fiber, int argCount, Value* args);
bool nativeStringReplace(objFiber* fiber, int argCount, Value* args);

bool nativeStringCharAt(objFiber* fiber, int argCount, Value* args);
bool nativeStringByteAt(objFiber* fiber, int argCount, Value* args);

bool nativeStringPos(objFiber* fiber, int argCount, Value* args);
bool nativeStringLastPos(objFiber* fiber, int argCount, Value* args);

bool nativeStringIsUpper(objFiber* fiber, int argCount, Value* args);
bool nativeStringIsLower(objFiber* fiber, int argCount, Value* args);

bool nativeStringLower(objFiber* fiber, int argCount, Value* args);
bool nativeStringUpper(objFiber* fiber, int argCount, Value* args);

bool nativeStringToDigits(objFiber* fiber, int argCount, Value* args);
#pragma endregion

//TODO: finish implementing these
#pragma region Files
bool nativeFileOpen(objFiber* fiber, int argCount, Value* args);
bool nativeFileClose(objFiber* fiber, int argCount, Value* args);
bool nativeFileExists(objFiber* fiber, int argCount, Value* args);

bool nativeFileReadString(objFiber* fiber, int argCount, Value* args);
bool nativeFileReadReal(objFiber* fiber, int argCount, Value* args);
bool nativeFileReadLn(objFiber* fiber, int argCount, Value* args);

bool nativeFileWriteString(objFiber* fiber, int argCount, Value* args);
bool nativeFileWriteReal(objFiber* fiber, int argCount, Value* args);
bool nativeFileWriteLn(objFiber* fiber, int argCount, Value* args);
#pragma endregion


bool nativeClock(objFiber* fiber, int argCount, Value* args);

bool nativeRandomRange(objFiber* fiber, int argCount, Value* args);
bool nativeRandomNumber(objFiber* fiber, int argCount, Value* args);
bool nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args);

bool nativeToString(objFiber* fiber, int argCount, Value* args);
bool nativeToReal(objFiber* fiber, int argCount, Value* args);

bool nativeInput(objFiber* fiber, int argCount, Value* args);
