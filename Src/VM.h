#pragma once

#include "common.h"
#include "compiler.h"
#include "object.h"
#include "hashTable.h"
#include "fiber.h"

class vm {
public:
	vm(compiler* current);
	~vm();

	void interpret(objFiber* fiber);
	void switchToFiber(objFiber* fiber);

	objFiber* curFiber;
	hashTable globals;
	objModule* curModule;
private:
	void defineNative(string name, NativeFn func, int arity);
};