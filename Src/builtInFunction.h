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
void nativeRandomRange(objFiber* fiber, int argCount, Value* args);
void nativeSetRandomSeed(objFiber* fiber, int argCount, Value* args);

void nativeToString(objFiber* fiber, int argCount, Value* args);
void nativeInput(objFiber* fiber, int argCount, Value* args);
