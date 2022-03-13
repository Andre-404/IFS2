#include "VM.h"
#include "debug.h"
#include "namespaces.h"
#include "builtInFunction.h"


using namespace global;

vm::vm(compiler* current) {
	if (!current->compiled) return;
	//we do this here because defineNative() mutates the globals field,
	//and if a collection happens we need to update all pointers in globals
	gc.VM = this;
	curFiber = nullptr;
	curModule = nullptr;
	toString = copyString("toString");
	//globals are set in the vm and the fibers access them by having a pointer to the VM
	defineNative("clock", nativeClock, 0);
	defineNative("randomRange", nativeRandomRange, 2);
	defineNative("setRandomSeed", nativeSetRandomSeed, 1);
	defineNative("string", nativeToString, 1);
	defineNative("input", nativeInput, 0);

	defineNative("floor", nativeFloor, 1);
	defineNative("ceil", nativeCeil, 1);
	defineNative("round", nativeRound, 1);

	defineNative("max", nativeMax, -1);
	defineNative("min", nativeMin, -1);
	defineNative("mean", nativeMean, -1);

	defineNative("sin", nativeSin, 1);
	defineNative("dsin", nativeDsin, 1);
	defineNative("cos", nativeCos, 1);
	defineNative("dcos", nativeDcos, 1);
	defineNative("tan", nativeTan, 1);
	defineNative("dtan", nativeDtan, 1);

	defineNative("logn", nativeLogn, 2);
	defineNative("log2", nativeLog2, 1);
	defineNative("log10", nativeLog10, 1);
	defineNative("ln", nativeLogE, 1);

	defineNative("pow", nativePow, 2);
	defineNative("sqrt", nativeSqrt, 1);
	defineNative("sqr", nativeSqr, 1);

	defineNative("arrayCreate", nativeArrayCreate, 1);
	defineNative("arrayResize", nativeArrayResize, 2);
	defineNative("arrayCopy", nativeArrayCopy, 1);
	defineNative("arrayPush", nativeArrayPush, 2);
	defineNative("arrayPop", nativeArrayPop, 1);
	defineNative("arrayInsert", nativeArrayInsert, 3);
	defineNative("arrayDelete", nativeArrayDelete, 2);
	defineNative("arrayLength", nativeArrayLength, 1);

	//using cachePtr because a relocation might happen while allocating closure or fiber
	objFunc* func = current->endFuncDecl();
	gc.cachePtr(func);
	gc.cachePtr(new objClosure(dynamic_cast<objFunc*>(gc.getCachedPtr())));
	objFiber* fiber = new objFiber(dynamic_cast<objClosure*>(gc.getCachedPtr()), this, 0);
	delete current;
	//every function will be accessible from the VM from now on, so there's no point in marking and tracing the compiler
	gc.compilerSession = nullptr;
	interpret(fiber);
}

vm::~vm() {
	global::gc.clear();
	global::gc.~GC();
	global::internedStrings.~hashTable();
}

void vm::switchToFiber(objFiber* fiber) {
	//when we first start up the VM curFiber is nullptr
	if (curFiber != nullptr) {
		curFiber->pause();
	}
	curFiber = fiber;
}

void vm::interpret(objFiber* fiber) {
	switchToFiber(fiber);

	while (curFiber != nullptr) {
		interpretResult result = curFiber->execute();
		//if we had a error(INTERPRETER_RUNTIME_ERROR) or  sucessfully ran all of the bytecode(INTERPRETER_OK) we quit
		if (result != interpretResult::INTERPRETER_PAUSED) return;
	}
}

void vm::defineNative(string name, NativeFn func, int arity) {
	//caching ptr in case a collection happens while allocating objNativeFn
	gc.cachePtr(copyString((char*)name.c_str(), name.length()));
	objNativeFn* fn = new objNativeFn(func, arity);

	globals.set(dynamic_cast<objString*>(gc.getCachedPtr()), OBJ_VAL(fn));
}
