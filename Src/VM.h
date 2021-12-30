#ifndef __IFS_VM
#define __IFS_VM

#include "common.h"
#include "compiler.h"
#include "object.h"
#include "hashTable.h"

enum class interpretResult {
	INTERPRETER_OK,
	INTERPRETER_RUNTIME_ERROR
};

#define STACK_MAX 256

class vm {
public:
	vm(compiler* current);
	~vm();
	uint8_t getOp(long _ip);
	interpretResult interpret(chunk* _chunk);
	interpretResult run();
	obj* objects;//linked list
private:
	long ip;
	chunk* curChunk;
	Value stack[STACK_MAX];
	Value* stackTop;
	hashTable globals;
	void concatenate();
	void push(Value val);
	Value pop();
	Value peek(int depth);
	void resetStack();
	void runtimeError(const char* format, ...);
	obj* appendObject(obj* _object);//append to the 'objects' list
	void freeObjects();
};


#endif // !__IFS_VM
