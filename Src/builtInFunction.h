#pragma once

#include "common.h"
#include "object.h"


#pragma region Arrays

void nativeArrayCreate(objFiber* fiber, int argCount, Value* args);
void nativeArrayCopy(objFiber* fiber, int argCount, Value* args);
void nativeArrayResize(objFiber* fiber, int argCount, Value* args);

void nativeArrayPush(objFiber* fiber, int argCount, Value* args);
void nativeArrayPop(objFiber* fiber, int argCount, Value* args);
void nativeArrayInsert(objFiber* fiber, int argCount, Value* args);
void nativeArrayDelete(objFiber* fiber, int argCount, Value* args);

void nativeArrayLength(objFiber* fiber, int argCount, Value* args);
#pragma endregion

void nativeClock(objFiber* fiber, int argCount, Value* args);

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


void nativeRandomRange(objFiber* fiber, int argCount, Value* args);
void nativeRandomNumber(objFiber* fiber, int argCount, Value* args);
void nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args);

void nativeToString(objFiber* fiber, int argCount, Value* args);
void nativeToReal(objFiber* fiber, int argCount, Value* args);

void nativeInput(objFiber* fiber, int argCount, Value* args);
