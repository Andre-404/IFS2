#ifndef __IFS_BUILTIN
#define __IFSBUILTIN
#include "common.h"
#include "object.h"


#pragma region Arrays

Value nativeArrayCreate(int argCount, Value* args);
Value nativeArrayCopy(int argCount, Value* args);
Value nativeArrayResize(int argCount, Value* args);

Value nativeArrayPush(int argCount, Value* args);
Value nativeArrayPop(int argCount, Value* args);
Value nativeArrayInsert(int argCount, Value* args);
Value nativeArrayDelete(int argCount, Value* args);

Value nativeArrayLength(int argCount, Value* args);
#pragma endregion

Value nativeClock(int argCount, Value* args);
Value nativeFloor(int argCount, Value* args);
Value nativeRandomRange(int argCount, Value* args);
Value nativeSetRandomSeed(int argCount, Value* args);

#endif // __IFS_BUILTIN
