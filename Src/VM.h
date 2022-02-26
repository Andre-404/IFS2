#pragma once

#include "common.h"
#include "compiler.h"
#include "object.h"
#include "hashTable.h"

enum class interpretResult {
	INTERPRETER_OK,
	INTERPRETER_RUNTIME_ERROR
};

struct callFrame {
	objClosure* closure;
	long ip;
	Value* slots;
};

#define FRAMES_MAX 128
#define STACK_MAX (FRAMES_MAX * 512)

class vm {
public:
	vm(compiler* current);
	~vm();
	uint8_t getOp(long _ip);
	interpretResult interpret(objFunc* _chunk);
	interpretResult run();
	Value stack[STACK_MAX];
	Value* stackTop;
	vector<objUpval*> openUpvals;
	callFrame frames[FRAMES_MAX];
	int frameCount;
	hashTable globals;
private:
	void concatenate();
	void push(Value val);
	Value pop();
	Value peek(int depth);

	void resetStack();
	interpretResult runtimeError(const char* format, ...);

	bool callValue(Value callee, int argCount);
	bool call(objClosure* function, int argCount);

	void defineNative(string name, NativeFn func, int arity);
	objUpval* captureUpvalue(Value* local);
	void closeUpvalues(Value* last);

	void defineMethod(objString* name);
	bool bindMethod(objClass* klass, objString* name);
	bool invoke(objString* methodName, int argCount);
	bool invokeFromClass(objClass* klass, objString* fieldName, int argCount);
};