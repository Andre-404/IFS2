#pragma once
#include "object.h"

#define FRAMES_MAX 128
#define STACK_MAX (FRAMES_MAX * 512)

struct callFrame {
	objClosure* closure;
	uInt64 ip;
	Value* slots;
	callFrame() : closure(nullptr), ip(0), slots(nullptr) {};
};

enum class interpretResult {
	INTERPRETER_OK,
	INTERPRETER_RUNTIME_ERROR,
	INTERPRETER_PAUSED,
};

#define RUNTIME_ERROR interpretResult::INTERPRETER_RUNTIME_ERROR;

enum class fiberState {
	NOT_STARTED,
	RUNNING,
	PAUSED,
	FINSIHED
};

string expectedType(string msg, Value val);

class objFiber : public obj {
public:
	objFiber(objClosure* _code, vm* _VM, uInt64 _startValuesNum);
	uInt64 startValuesNum;

	interpretResult execute();
	void pause();
	//stuff used by native functions
	void transferValue(Value val);
	void reduceStack(uInt nToPop);

	//getters
	bool isRunning() { return state == fiberState::RUNNING; }
	bool isFinished() { return state == fiberState::FINSIHED; }
	bool hasStarted() { return state != fiberState::NOT_STARTED; }



	//Memory stuff
	void move(byte* to) {};
	size_t getSize() { return sizeof(objFiber); }
	void updatePtrs();
	void trace(std::vector<managed*>& stack);

	void* operator new(size_t size);
private:
	//VM stuff
	uint8_t getOp(long _ip);
	void concatenate();
	void push(Value val);
	Value pop();
	Value peek(int depth);
	bool friend pushValToStr(objFiber* fiber, Value val);

	void resetStack();
	interpretResult runtimeError(const char* format, ...);

	bool callValue(Value callee, int argCount);
	bool call(objClosure* function, int argCount);


	objUpval* captureUpvalue(Value* local);
	void closeUpvalues(Value* last);

	void defineMethod(objString* name);
	bool bindMethod(objClass* klass, objString* name);
	bool invoke(objString* methodName, int argCount);
	bool invokeFromClass(objClass* klass, objString* fieldName, int argCount);

	Value stack[STACK_MAX];
	Value* stackTop;
	std::vector<objUpval*> openUpvals;
	callFrame frames[FRAMES_MAX];
	int frameCount;
	objClosure* codeBlock;
	//fiber stuff
	vm* VM;
	fiberState state;
	objFiber* prevFiber;
};

