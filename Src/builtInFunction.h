#pragma once

#include "common.h"
#include "object.h"


#pragma region Arrays

void nativeArrayCreate(objFiber* fiber, int argCount, Value* args);
void nativeArrayCopy(objFiber* fiber, int argCount, Value* args);
void nativeArrayResize(objFiber* fiber, int argCount, Value* args);
void nativeArrayFill(objFiber* fiber, int argCount, Value* args);

void nativeArrayPush(objFiber* fiber, int argCount, Value* args);
void nativeArrayPop(objFiber* fiber, int argCount, Value* args);
void nativeArrayInsert(objFiber* fiber, int argCount, Value* args);
void nativeArrayDelete(objFiber* fiber, int argCount, Value* args);

void nativeArrayLength(objFiber* fiber, int argCount, Value* args);
#pragma endregion

#pragma region Math
void nativeFloor(objFiber* fiber, int argCount, Value* args);
void nativeCeil(objFiber* fiber, int argCount, Value* args);
void nativeRound(objFiber* fiber, int argCount, Value* args);

void nativeMax(objFiber* fiber, int argCount, Value* args);
void nativeMin(objFiber* fiber, int argCount, Value* args);
void nativeMean(objFiber* fiber, int argCount, Value* args);

void nativeSin(objFiber* fiber, int argCount, Value* args);
void nativeDsin(objFiber* fiber, int argCount, Value* args);
void nativeCos(objFiber* fiber, int argCount, Value* args);
void nativeDcos(objFiber* fiber, int argCount, Value* args);
void nativeTan(objFiber* fiber, int argCount, Value* args);
void nativeDtan(objFiber* fiber, int argCount, Value* args);

void nativeLogn(objFiber* fiber, int argCount, Value* args);
void nativeLog10(objFiber* fiber, int argCount, Value* args);
void nativeLog2(objFiber* fiber, int argCount, Value* args);
void nativeLogE(objFiber* fiber, int argCount, Value* args);

void nativePow(objFiber* fiber, int argCount, Value* args);
void nativeSqrt(objFiber* fiber, int argCount, Value* args);
void nativeSqr(objFiber* fiber, int argCount, Value* args);
#pragma endregion

#pragma region Types
void nativeIsNumber(objFiber* fiber, int argCount, Value* args);
void nativeIsNil(objFiber* fiber, int argCount, Value* args);
void nativeIsBool(objFiber* fiber, int argCount, Value* args);
void nativeIsString(objFiber* fiber, int argCount, Value* args);
void nativeIsArray(objFiber* fiber, int argCount, Value* args);
void nativeIsStruct(objFiber* fiber, int argCount, Value* args);
void nativeIsInstance(objFiber* fiber, int argCount, Value* args);
void nativeIsClass(objFiber* fiber, int argCount, Value* args);
void nativeIsFunction(objFiber* fiber, int argCount, Value* args);
void nativeIsMethod(objFiber* fiber, int argCount, Value* args);
void nativeIsModule(objFiber* fiber, int argCount, Value* args);
void nativeIsFiber(objFiber* fiber, int argCount, Value* args);
#pragma endregion

#pragma region Strings
void nativeStringLength(objFiber* fiber, int argCount, Value* args);

void nativeStringInsert(objFiber* fiber, int argCount, Value* args);
void nativeStringDelete(objFiber* fiber, int argCount, Value* args);
void nativeStringSubstr(objFiber* fiber, int argCount, Value* args);
void nativeStringReplace(objFiber* fiber, int argCount, Value* args);

void nativeStringCharAt(objFiber* fiber, int argCount, Value* args);
void nativeStringByteAt(objFiber* fiber, int argCount, Value* args);

void nativeStringPos(objFiber* fiber, int argCount, Value* args);
void nativeStringLastPos(objFiber* fiber, int argCount, Value* args);

void nativeStringIsUpper(objFiber* fiber, int argCount, Value* args);
void nativeStringIsLower(objFiber* fiber, int argCount, Value* args);

void nativeStringLower(objFiber* fiber, int argCount, Value* args);
void nativeStringUpper(objFiber* fiber, int argCount, Value* args);

void nativeStringToDigits(objFiber* fiber, int argCount, Value* args);
#pragma endregion

#pragma region Files
void nativeFileOpen(objFiber* fiber, int argCount, Value* args);
void nativeFileClose(objFiber* fiber, int argCount, Value* args);
void nativeFileExists(objFiber* fiber, int argCount, Value* args);

void nativeFileReadString(objFiber* fiber, int argCount, Value* args);
void nativeFileReadReal(objFiber* fiber, int argCount, Value* args);
void nativeFileReadLn(objFiber* fiber, int argCount, Value* args);

void nativeFileWriteString(objFiber* fiber, int argCount, Value* args);
void nativeFileWriteReal(objFiber* fiber, int argCount, Value* args);
void nativeFileWriteLn(objFiber* fiber, int argCount, Value* args);
#pragma endregion


void nativeClock(objFiber* fiber, int argCount, Value* args);

void nativeRandomRange(objFiber* fiber, int argCount, Value* args);
void nativeRandomNumber(objFiber* fiber, int argCount, Value* args);
void nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args);

void nativeToString(objFiber* fiber, int argCount, Value* args);
void nativeToReal(objFiber* fiber, int argCount, Value* args);

void nativeInput(objFiber* fiber, int argCount, Value* args);
