#include "VM.h"
#include "debug.h"
#include "namespaces.h"
#include "builtInFunction.h"
#include <stdarg.h>

using namespace global;

#define RUNTIME_ERROR interpretResult::INTERPRETER_RUNTIME_ERROR;

vm::vm(compiler* current) {
	frameCount = 0;
	resetStack();
	if (!current->compiled) return;
	//we do this here because defineNative() mutates the globals field,
	//and if a collection happens we need to update all pointers in globals
	global::gc.VM = this;
	defineNative("clock", nativeClock, 0);
	defineNative("floor", nativeFloor, 1);
	defineNative("randomRange", nativeRandomRange, 2);
	defineNative("setRandomSeed", nativeSetRandomSeed, 1);

	defineNative("arrayCreate", nativeArrayCreate, 1);
	defineNative("arrayResize", nativeArrayResize, 2);
	defineNative("arrayCopy", nativeArrayCopy, 1);
	defineNative("arrayPush", nativeArrayPush, 2);
	defineNative("arrayPop", nativeArrayPop, 1);
	defineNative("arrayInsert", nativeArrayInsert, 3);
	defineNative("arrayDelete", nativeArrayDelete, 2);
	defineNative("arrayLength", nativeArrayLength, 1);

	objFunc* func = current->endFuncDecl();
	delete current;
	//every function will be accessible from the VM from now on, so there's no point in marking and tracing the compiler
	global::gc.compiling = nullptr;
	interpret(func);
}

vm::~vm() {
	global::gc.clear();
	global::gc.~GC();
	global::internedStrings.~hashTable();
}

#pragma region Helpers

uint8_t vm::getOp(long _ip) {
	return frames[frameCount - 1].closure->func->body.code[_ip];
}

void vm::push(Value val) {
	*stackTop = val;
	stackTop++;
}

Value vm::pop() {
	stackTop--;
	return *stackTop;
}

void vm::resetStack() {
	stackTop = stack;
}

Value vm::peek(int depth) {
	return stackTop[-1 - depth];
}

interpretResult vm::runtimeError(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	//prints callstack
	for (int i = frameCount - 1; i >= 0; i--) {
		callFrame* frame = &frames[i];
		objFunc* function = frame->closure->func;
		size_t instruction = frame->ip - 1;
		fprintf(stderr, "[line %d] in ", function->body.lines[instruction]);
		std::cout << (function->name == NULL ? "script" : function->name->str) << "()\n";
	}
	resetStack();
	return RUNTIME_ERROR;
}

static bool isFalsey(Value value) {
	return ((IS_BOOL(value) && !AS_BOOL(value)) || IS_NIL(value));
}

//concats 2 strings by allocating a new char* buffer using new and uses that to make a new objString
void vm::concatenate() {
	objString* b = AS_STRING(peek(0));
	objString* a = AS_STRING(peek(1));
	
	uInt newLen = a->length + b->length;
	//this pointer is deleted by takeString, possible optimization is to avoid this all together and alloc directly to heap
	char* temp = new char[newLen + 1];
	memcpy(temp, a->str, a->length);
	memcpy(temp + a->length, b->str, b->length);
	temp[newLen] = '\0';
	objString* result = takeString(temp, newLen);
	pop();
	pop();
	push(OBJ_VAL(result));
}

bool vm::callValue(Value callee, int argCount) {
	if (IS_OBJ(callee)) {
		switch (OBJ_TYPE(callee)) {
		case OBJ_CLOSURE:
			return call(AS_CLOSURE(callee), argCount);
		case OBJ_NATIVE: {
			int arity = AS_NATIVE(callee)->arity;
			if (argCount != arity) {
				runtimeError("Expected %d arguments but got %d.", arity, argCount);
				return false;
			}
			NativeFn native = AS_NATIVE(callee)->func;
			Value result = NIL_VAL();
			//native functions throw strings when a error has occured
			try {
				result = native(argCount, stackTop - argCount);
			}catch (const char* str) {
				const char* name = globals.getKey(callee)->str;//gets the name of the native func
				runtimeError("Error in %s: %s", name, str);
				return false;
			}
			
			stackTop -= argCount + 1;
			push(result);
			return true;
		}
		case OBJ_CLASS: {
			//we do this so if a GC runs we safely update all the pointers(since the stack is considered a root)
			push(callee);
			stackTop[-argCount - 2] = OBJ_VAL(new objInstance(AS_CLASS(peek(0))));
			objClass* klass = AS_CLASS(pop());
			Value initializer;
			if (klass->methods.get(klass->name, &initializer)) {
				return call(AS_CLOSURE(initializer), argCount);
			} else if (argCount != 0) {
				runtimeError("Expected 0 arguments for class initalization but got %d.", argCount);
				return false;
			}
			return true;
		}
		case OBJ_BOUND_METHOD: {
			//puts the receiver instance in the 0th slot('this' points to the 0th slot)
			objBoundMethod* bound = AS_BOUND_METHOD(callee);
			stackTop[-argCount - 1] = bound->receiver;
			return call(bound->method, argCount);
		}
		default:
			break; // Non-callable object type.
		}
	}
	runtimeError("Can only call functions and classes.");
	return false;
}

bool vm::call(objClosure* closure, int argCount) {
	if (argCount != closure->func->arity) {
		runtimeError("Expected %d arguments but got %d.", closure->func->arity, argCount);
		return false;
	}

	if (frameCount == FRAMES_MAX) {
		runtimeError("Stack overflow.");
		return false;
	}

	callFrame* frame = &frames[frameCount++];
	frame->closure = closure;
	frame->ip = 0;
	frame->slots = stackTop - argCount - 1;
	return true;

}

void vm::defineNative(string name, NativeFn func, int arity) {
	//pushing the values because of GC
	push(OBJ_VAL(copyString((char*)name.c_str(), name.length())));
	push(OBJ_VAL(new objNativeFn(func, arity)));

	globals.set(AS_STRING(stack[0]), stack[1]);

	pop();
	pop();
}

objUpval* vm::captureUpvalue(Value* local) {
	//first checks to see if there already exists a upvalue that points to stack location 'local'
	for (int i = openUpvals.size() - 1; i >= 0; i--) {
		if (openUpvals[i]->location >= local) {
			if (openUpvals[i]->location == local) return openUpvals[i];
		}
		else break;
	}
	//if not, create a new upvalue
	objUpval* createdUpval = new objUpval(local);
	openUpvals.push_back(createdUpval);
	return createdUpval;
}

void vm::closeUpvalues(Value* last) {
	objUpval* upval;
	//close every value on the stack until last
	for (int i = openUpvals.size() - 1; i >= 0; i--) {
		upval = openUpvals[i];
		if (upval->location >= last) {
			upval->closed = *upval->location;
			upval->location = &upval->closed;
			upval->isOpen = false;//this check is used by the GC
		}
		else break;
	}
}

void vm::defineMethod(objString* name) {
	//no need to typecheck since the compiler made sure to emit code in this order
	Value method = peek(0);
	objClass* klass = AS_CLASS(peek(1));
	klass->methods.set(name, method);
	//we only pop the method, since other methods we're compiling will also need to know their class
	pop();
}

bool vm::bindMethod(objClass* klass, objString* name) {
	//At the start the instance whose method we're binding needs to be on top of the stack
	Value method;
	if (!klass->methods.get(name, &method)) {
		runtimeError("Undefined property '%s'.", name->str);
		return false;
	}
	//we need to push the method to the stack because if a GC collection happens the ptr inside of method becomes invalid
	//easiest way for it to update is to push it onto the stack
	push(method);
	//peek(1) because the method(closure) itself is on top of the stack right now
	objBoundMethod* bound = new objBoundMethod(peek(1), nullptr);
	//make sure to pop the closure to maintain stack discipline and to get the updated value(if it changed)
	method = pop();
	bound->method = AS_CLOSURE(method);
	//pop the receiver instance
	pop();
	push(OBJ_VAL(bound));
	return true;
}

bool vm::invoke(objString* fieldName, int argCount) {
	Value receiver = peek(argCount);
	if (!IS_INSTANCE(receiver)) {
		runtimeError("Only instances have methods.");
		return false;
	}

	objInstance* instance = AS_INSTANCE(receiver);
	Value value;
	//this is here because invoke can also be used on functions stored in a instances field
	if (instance->table.get(fieldName, &value)) {
		stackTop[-argCount - 1] = value;
		return callValue(value, argCount);
	}
	//this check is used because we also use objInstance to represent struct literals
	//and if this instance is a struct it can only contain functions inside it's field table
	if (instance->klass == nullptr) {
		runtimeError("Undefined property '%s'.", fieldName->str);
		return false;
	}

	return invokeFromClass(instance->klass, fieldName, argCount);
}

bool vm::invokeFromClass(objClass* klass, objString* name, int argCount) {
	Value method;
	if (!klass->methods.get(name, &method)) {
		runtimeError("Undefined property '%s'.", name->str);
		return false;
	}
	//the bottom of the call stack will contain the receiver instance
	return call(AS_CLOSURE(method), argCount);
}
#pragma endregion



interpretResult vm::interpret(objFunc* func) {
	//create a new function(implicit one for the whole code), and put it on the call stack
	if (func == NULL) return interpretResult::INTERPRETER_RUNTIME_ERROR;
	push(OBJ_VAL(func));//doing this because of GC
	//we use peek to avoid cached pointers
	objClosure* closure = new objClosure(AS_FUNCTION(peek(0)));
	pop();
	push(OBJ_VAL(closure));
	call(closure, 0);
	return run();
}

interpretResult vm::run() {
	callFrame* frame = &frames[frameCount - 1];
	#pragma region Macros
	#define READ_BYTE() (getOp(frame->ip++))
	#define READ_SHORT() (frame->ip += 2, (uint16_t)((getOp(frame->ip-2) << 8) | getOp(frame->ip-1)))
	#define READ_24BIT() (frame->ip += 3, (uInt)(getOp(frame->ip-3) | (getOp(frame->ip-2) << 8) | (getOp(frame->ip-1) << 16)))
	#define READ_CONSTANT() (frames[frameCount - 1].closure->func->body.constants[READ_BYTE()])
	#define READ_CONSTANT_LONG() (frames[frameCount - 1].closure->func->body.constants[READ_24BIT()])
	#define READ_STRING() AS_STRING(READ_CONSTANT())
	#define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())
	#define BINARY_OP(valueType, op) \
		do { \
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			return runtimeError("Operands must be numbers."); \
			} \
			double b = AS_NUMBER(pop()); \
			double a = AS_NUMBER(pop()); \
			push(valueType(a op b)); \
		} while (false)

	#define INT_BINARY_OP(valueType, op)\
		do {\
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return interpretResult::INTERPRETER_RUNTIME_ERROR; \
			} \
			double b = AS_NUMBER(pop()); \
			double a = AS_NUMBER(pop()); \
			push(valueType((double)((int)a op (int)b))); \
		} while (false)

	#ifdef DEBUG_TRACE_EXECUTION
		std::cout << "-------------Code execution starts-------------\n";
	#endif // DEBUG_TRACE_EXECUTION
	#pragma endregion
	
	while (true) {
		#ifdef DEBUG_TRACE_EXECUTION
		std::cout << "          ";
		for (Value* slot = stack; slot < stackTop; slot++) {
			std::cout << "[";
			printValue(*slot);
			std::cout << "] ";
		}
		std::cout << "\n";
		disassembleInstruction(&frame->closure->func->body, frames[frameCount - 1].ip);
		#endif

		uint8_t instruction;
		switch (instruction = READ_BYTE()) {

		#pragma region Helpers
		case OP_POP:
			pop();
			break;
		case OP_POPN: {
			uint8_t nToPop = READ_BYTE();
			int i = 0;
			while (i < nToPop) {
				pop();
				i++;
			}
			break;
		}
		#pragma endregion

		#pragma region Constants
		case OP_CONSTANT: {
			Value constant = READ_CONSTANT();
			push(constant);
			break;
		}
		case OP_CONSTANT_LONG: {
			Value constant = READ_CONSTANT_LONG();
			push(constant);
			break;
		}
		case OP_NIL: push(NIL_VAL()); break;
		case OP_TRUE: push(BOOL_VAL(true)); break;
		case OP_FALSE: push(BOOL_VAL(false)); break;
		#pragma endregion

		#pragma region Unary
		case OP_NEGATE:
		if (!IS_NUMBER(peek(0))) {
			return runtimeError("Operand must be a number.");
		}
		push(NUMBER_VAL(-AS_NUMBER(pop())));
		break;
		case OP_NOT:
			push(BOOL_VAL(isFalsey(pop())));
			break;
		case OP_BIN_NOT: {
			if (!IS_NUMBER(peek(0))) {
				return runtimeError("Operand must be a number.");
			}
			int num = AS_NUMBER(pop());
			num = ~num;
			push(NUMBER_VAL((double)num));
			break;
		}
		#pragma endregion

		#pragma region Binary
		case OP_ADD: {
			if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
				concatenate();
			}
			else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
				double b = AS_NUMBER(pop());
				double a = AS_NUMBER(pop());
				push(NUMBER_VAL(a + b));
			}
			else {
				return runtimeError("Operands must be two numbers or two strings.");
			}
			break;
		}
		case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
		case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
		case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, / ); break;
		case OP_MOD:	  INT_BINARY_OP(NUMBER_VAL, %); break;
		case OP_BITSHIFT_LEFT: INT_BINARY_OP(NUMBER_VAL, << ); break;
		case OP_BITSHIFT_RIGHT: INT_BINARY_OP(NUMBER_VAL, >> ); break;
		case OP_BITWISE_AND: INT_BINARY_OP(NUMBER_VAL, &); break;
		case OP_BITWISE_OR: INT_BINARY_OP(NUMBER_VAL, | ); break;
		case OP_BITWISE_XOR: INT_BINARY_OP(NUMBER_VAL, ^); break;
		case OP_ADD_1: {
			if (IS_NUMBER(peek(0))) {
				Value num = pop();
				num.as.num++;
				push(num);
			}
			else return runtimeError("Operands must be two numbers.");
			break;
		}
		case OP_SUBTRACT_1: {
			if (IS_NUMBER(peek(0))) {
				Value num = pop();
				num.as.num--;
				push(num);
			}else return runtimeError("Operands must be two numbers.");
			break;
		}
		#pragma endregion

		#pragma region Binary that returns bools
		case OP_EQUAL: {
			Value b = pop();
			Value a = pop();
			push(BOOL_VAL(valuesEqual(a, b)));
			break;
		}
		case OP_NOT_EQUAL: {
			Value b = pop();
			Value a = pop();
			push(BOOL_VAL(!valuesEqual(a, b)));
			break;
		}
		case OP_GREATER: BINARY_OP(BOOL_VAL, > ); break;
		case OP_LESS: {
			Value a = peek(1);
			Value b = peek(0);
			BINARY_OP(BOOL_VAL, < ); break; }
		case OP_GREATER_EQUAL: {
			//Have to do this because of floating point comparisons
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
				return runtimeError("Operands must be numbers.");
			}
			double b = AS_NUMBER(pop());
			double a = AS_NUMBER(pop());
			if (a > b || FLOAT_EQ(a, b)) push(BOOL_VAL(true));
			else push(BOOL_VAL(false));
			break;
		}
		case OP_LESS_EQUAL: {
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
				return runtimeError("Operands must be numbers.");
			}
			double b = AS_NUMBER(pop());
			double a = AS_NUMBER(pop());
			if (a < b || FLOAT_EQ(a, b)) push(BOOL_VAL(true));
			else push(BOOL_VAL(false));
			break;
		}
		#pragma endregion

		#pragma region Statements
		case OP_PRINT: {
			Value toPrint = peek(0);
			if (!IS_INSTANCE(toPrint)) {
				//there is always a OP_POP after OP_PRINT, but if we know we're not going to call a toString method of some object,
				//we pop the value and skip over the OP_POP
				printValue(pop());
				READ_BYTE();
			}else {
				Value instVal = peek(0);

				objInstance* inst = AS_INSTANCE(instVal);
				Value val;

				if (inst->klass == nullptr) {
					//there is always a OP_POP after OP_PRINT, but if we know we're not going to call a toString method of some object,
					//we pop the value and skip over the OP_POP
					printObject(pop());
					READ_BYTE();
					break;
				}
				//TODO: optimize this, if no object has a "toString" method this slows down the code massivly
				objString* temp = copyString("toString", 8);
				if (!invoke(temp, 0)) {
					return RUNTIME_ERROR;
				}
				frame = &frames[frameCount - 1];
				break;
			}
			std::cout << "\n";
			break;
		}

		case OP_DEFINE_GLOBAL: {
			objString* name = READ_STRING();
			globals.set(name, peek(0));
			pop();
			break;
		}
		case OP_DEFINE_GLOBAL_LONG: {
			objString* name = READ_STRING_LONG();
			globals.set(name, peek(0));
			pop();
			break;
		}

		case OP_GET_GLOBAL: {
			objString* name = READ_STRING();
			Value value;
			if (!globals.get(name, &value)){
				return runtimeError("Undefined variable '%s'.", name->str);
			}
			push(value);
			break;
		}
		case OP_GET_GLOBAL_LONG: {
			objString* name = READ_STRING_LONG();
			Value value;
			if (!globals.get(name, &value)) {
				return runtimeError("Undefined variable '%s'.", name->str);
			}
			push(value);
			break;
		}

		case OP_SET_GLOBAL: {
			objString* name = READ_STRING();
			if (globals.set(name, peek(0))) {
				globals.del(name);
				return runtimeError("Undefined variable '%s'.", name->str);
			}
			break;
		}
		case OP_SET_GLOBAL_LONG: {
			objString* name = READ_STRING_LONG();
			if (globals.set(name, peek(0))) {
				globals.del(name);
				return runtimeError("Undefined variable '%s'.", name->str);
			}
			break;
		}

		case OP_GET_LOCAL: {
			uint8_t slot = READ_BYTE();
			push(frame->slots[slot]);
			break;
		}

		case OP_SET_LOCAL: {
			uint8_t slot = READ_BYTE();
			frame->slots[slot] = peek(0);
			break;
		}
		//upvals[slot]->location can be a pointer to either a stack slot, or a closed upvalue in a functions closure
		case OP_GET_UPVALUE: {
			uint8_t slot = READ_BYTE();
			push(*frame->closure->upvals[slot]->location);
			break;
		}

		case OP_SET_UPVALUE: {
			uint8_t slot = READ_BYTE();
			*frame->closure->upvals[slot]->location = peek(0);
			break;
		}

		case OP_CLOSE_UPVALUE: {
			closeUpvalues(stackTop - 1);
			pop();
			break;
		}
		#pragma endregion

		#pragma region Control flow
		case OP_JUMP_IF_TRUE: {
			uint16_t offset = READ_SHORT();
			if (!isFalsey(peek(0))) frame->ip += offset;
			break;
		}
		case OP_JUMP_IF_FALSE: {
			uint16_t offset = READ_SHORT();
			if (isFalsey(peek(0))) frame->ip += offset;
			break;
		}
		case OP_JUMP_IF_FALSE_POP: {
			uint16_t offset = READ_SHORT();
			if (isFalsey(pop())) frame->ip += offset;
			break;
		}
		case OP_JUMP: {
			uint16_t offset = READ_SHORT();
			frame->ip += offset;
			break;
		}
		case OP_LOOP: {
			uint16_t offset = READ_SHORT();
			frame->ip -= offset;
			break;
		}
		case OP_BREAK: {
			uint16_t toPop = READ_SHORT();
			int i = 0;
			while (i < toPop) {
				pop();
				i++;
			}
			uint16_t offset = READ_SHORT();
			frame->ip += offset;
			break;
		}
		case OP_SWITCH: {
			if (IS_STRING(peek(0)) || IS_NUMBER(peek(0))) {
				int pos = READ_BYTE();
				switchTable& _table = frames[frameCount - 1].closure->func->body.switchTables[pos];
				//default jump exists for every switch, and if it's not user defined it jumps to the end of switch
				switch (_table.type) {
				case switchType::NUM: {
					//if it's a all number switch stmt, and we have something that isn't a number, we immediatelly jump to default
					if (IS_NUMBER(peek(0))) {
						int num = AS_NUMBER(pop());
						long jumpLength = _table.getJump(num);
						if (jumpLength != -1) {
							frame->ip += jumpLength;
							break;
						}
					}
					frame->ip += _table.defaultJump;
					break;
				}
				case switchType::STRING: {
					if (IS_STRING(peek(0))) {
						objString* str = AS_STRING(pop());
						string key;
						key.assign(str->str, str->length + 1);
						long jumpLength = _table.getJump(key);
						if (jumpLength != -1) {
							frame->ip += jumpLength;
							break;
						}
					}
					frame->ip += _table.defaultJump;
					break;
				}
				case switchType::MIXED: {
					string str;
					//this can be either a number or a string, so we need more checks
					if (IS_STRING(peek(0))) {
						objString* key = AS_STRING(pop());
						str.assign(key->str, key->length + 1);
					}else str = std::to_string((int)AS_NUMBER(pop()));
					long jumpLength = _table.getJump(str);
					if (jumpLength != -1) {
						frame->ip += jumpLength;
						break;
					}
					frame->ip += _table.defaultJump;
					break;
				}
				}
			}else {
				return runtimeError("Switch expression can be only string or number.");
			}
			break;
		}
		#pragma endregion

		#pragma region Functions
		case OP_CALL: {
			//how many values are on the stack right now
			int argCount = READ_BYTE();
			if (!callValue(peek(argCount), argCount)) {
				return RUNTIME_ERROR;
			}
			//if the call is succesful, there is a new call frame, so we need to update the pointer
			frame = &frames[frameCount - 1];
			break;
		}
		case OP_RETURN: {
			Value result = pop();
			closeUpvalues(frame->slots);
			frameCount--;
			//if we're returning from the implicit funcition
			if (frameCount == 0) {
				pop();
				return interpretResult::INTERPRETER_OK;
			}

			stackTop = frame->slots;
			push(result);
			//if the call is succesful, there is a new call frame, so we need to update the pointer
			frame = &frames[frameCount - 1];
			break;
		}
		case OP_CLOSURE: {
			//doing this to avoid cached pointers
			objClosure* closure = new objClosure(AS_FUNCTION(READ_CONSTANT()));
			push(OBJ_VAL(closure));
			for (int i = 0; i < closure->upvals.size(); i++) {
				uint8_t isLocal = READ_BYTE();
				uint8_t index = READ_BYTE();
				if (isLocal) {
					closure->upvals[i] = captureUpvalue(frame->slots + index);
				}else {
					closure->upvals[i] = frame->closure->upvals[index];
				}
				//this serves to satisfy the GC, since we can't have any cached pointers when we collect
				closure = AS_CLOSURE(peek(0));
			}
			break;
		}
		case OP_CLOSURE_LONG: {
			//doing this to avoid cached pointers
			objClosure* closure = new objClosure(AS_FUNCTION(READ_CONSTANT_LONG()));
			push(OBJ_VAL(closure));
			for (int i = 0; i < closure->upvals.size(); i++) {
				uint8_t isLocal = READ_BYTE();
				uint8_t index = READ_BYTE();
				if (isLocal) {
					closure->upvals[i] = captureUpvalue(frame->slots + index);
				}
				else {
					closure->upvals[i] = frame->closure->upvals[index];
				}
				//this serves to satisfy the GC, since we can't have any cached pointers when we collect
				closure = AS_CLOSURE(peek(0));
			}
			break;
		}
		#pragma endregion

		#pragma region Objects, arrays and maps
		case OP_CREATE_ARRAY: {
			uInt64 size = READ_BYTE();
			uInt64 i = 0;
			objArray* arr = new objArray(size);
			while (i < size) {
				//size-i to because the values on the stack are in reverse order compared to how they're supposed to be in a array
				Value val = pop();
				//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
				if (IS_OBJ(val)) arr->numOfHeapPtr++;

				arr->values[size - i - 1] = val;
				i++;
			}
			push(OBJ_VAL(arr));
			break;
		}

		case OP_GET: {
			//structs and objects also get their own OP_GET_PROPERTY operator for access using '.'
			//use peek because in case this is a get call to a instance that has a defined "access" method
			//we want to use these 2 values as args and receiver
			Value field = peek(0);
			Value callee = peek(1);
			if (!IS_OBJ(callee))
				runtimeError("Expected a array or struct, got %s.", callee.type == VAL_NUM ? "number" : callee.type == VAL_NIL ? "nil" : "bool");
			
			switch (AS_OBJ(callee)->type) {
			case OBJ_ARRAY: {
				//we don't pop above because this might be a get call to a class that has a defined access method
				//but in order to maintain stack balance we need to get rid of these 2 vars if this is not a get call to a access method
				pop();
				pop();
				if (!IS_NUMBER(field)) return runtimeError("Index must be a number.");
				double index = AS_NUMBER(field);
				objArray* arr = AS_ARRAY(callee);
				//Trying to access a variable using a float is a error
				if ((uInt64)index != index) return runtimeError("Expected interger, got float.");
				if (index < 0 || index > arr->values.count() - 1)
					return runtimeError("Index %d outside of range [0, %d].", (uInt64)index, AS_ARRAY(callee)->values.count() - 1);

				push(arr->values[(uInt64)index]);
				break;
			}
			case OBJ_INSTANCE: {
				//check if this instance has a access method, and if it does it transfer control over to the user defined method
				//the 'field' value becomes the argument for the method 
				objString* temp = copyString("access", 6);
				if (!invoke(temp, 1)) {
					return RUNTIME_ERROR;
				}else {
					frame = &frames[frameCount - 1];
					break;
				}
				//pop if this instance doesn't contain a access method to maintain stack balance
				pop();
				pop();
				if (!IS_STRING(field)) return runtimeError("Expected a string for field name.");
				
				objInstance* instance = AS_INSTANCE(callee);
				objString* name = AS_STRING(field);
				Value value;

				if (instance->table.get(name, &value)) {
					push(value);
					break;
				}

				if (instance->klass && bindMethod(instance->klass, name)) break;
				//if the field doesn't exist, we push nil as a sentinel value, since this is not considered a runtime error
				push(NIL_VAL());
				break;
			}
			default:
				return runtimeError("Expected a array or struct.");
			}
			break;
		}

		case OP_SET: {
			//structs and objects also get their own OP_SET_PROPERTY operator for setting using '.'
			Value val = peek(0);
			Value field = peek(1);
			Value callee = peek(2);
			//if we encounter a user defined set expr, we will want to exit early and not pop all of the above values as they will be used as args
			bool earlyExit = false;

			if (!IS_OBJ(callee))
				runtimeError("Expected a array or struct, got %s.", callee.type == VAL_NUM ? "number" : callee.type == VAL_NIL ? "nil" : "bool");
			
			switch (AS_OBJ(callee)->type) {
			case OBJ_ARRAY: {
				objArray* arr = AS_ARRAY(callee);

				if (!IS_NUMBER(field)) return runtimeError("Index has to be a number");
				double index = AS_NUMBER(field);
				//accessing array with a float is a error
				if (index != (uInt64)index) return runtimeError("Index has to be a integer.");
				if (index < 0 || index > arr->values.count() - 1)
					return runtimeError("Index %d outside of range [0, %d].", (uInt64)index, arr->values.count() - 1);

				//if numOfHeapPtr is 0 we don't trace or update the array when garbage collecting
				if (IS_OBJ(val)) arr->numOfHeapPtr++;
				else if (IS_OBJ(arr->values[index])) arr->numOfHeapPtr--;
				arr->values[(uInt64)index] = val;
				break;
				}
			case OBJ_INSTANCE: {
				//check if this instance has a defined set method, if it does, transfer control to it and set the field and value as args to the method
				objString* temp = copyString("set", 3);
				if (!invoke(temp, 2)) {
					return RUNTIME_ERROR;
				}
				else {
					frame = &frames[frameCount - 1];
					earlyExit = true;
					break;
				}
				//if the instance doesn't have a defined set method, proceed as normal
				if (!IS_STRING(field)) return runtimeError("Expected a string for field name.");

				objInstance* instance = AS_INSTANCE(callee);
				objString* str = AS_STRING(field);
				//settting will always succeed, and we don't care if we're overriding an existing field, or creating a new one
				instance->table.set(str, val);
				break;
			}
			default:
				return runtimeError("Expected a array or struct.");
			}
			if (earlyExit) break;
			//we want only the value to remain on the stack, since set is a assignment expr
			pop();
			pop();
			pop();
			push(val);
			break;
		}

		case OP_CLASS: {
			push(OBJ_VAL(new objClass(READ_STRING_LONG())));
			break;
		}

		case OP_GET_PROPERTY: {
			if (!IS_INSTANCE(peek(0))) {
				runtimeError("Only instances/structs have properties.");
				return RUNTIME_ERROR;
			}

			objInstance* instance = AS_INSTANCE(peek(0));
			objString* name = READ_STRING();

			Value value;
			if (instance->table.get(name, &value)) {
				pop(); // Instance.
				push(value);
				break;
			}
			//first check is because structs(instances with no class) are also represented using objInstance
			if (instance->klass && bindMethod(instance->klass, name)) break;
			//if 'name' isn't a field property, nor is it a method, we push nil as a sentinel value
			push(NIL_VAL());
			break;
		}
		case OP_GET_PROPERTY_LONG: {
			if (!IS_INSTANCE(peek(0))) {
				runtimeError("Only instances/structs have properties.");
				return RUNTIME_ERROR;
			}

			objInstance* instance = AS_INSTANCE(peek(0));
			objString* name = READ_STRING_LONG();

			Value value;
			if (instance->table.get(name, &value)) {
				pop(); // Instance.
				push(value);
				break;
			}
			//first check is because structs(instances with no class) are also represented using objInstance
			if (instance->klass && bindMethod(instance->klass, name)) break;
			//if 'name' isn't a field property, nor is it a method, we push nil as a sentinel value
			push(NIL_VAL());
			break;
		}

		case OP_SET_PROPERTY: {
			if (!IS_INSTANCE(peek(1))) {
				runtimeError("Only instances have fields.");
				return RUNTIME_ERROR;
			}

			objInstance* instance = AS_INSTANCE(peek(1));
			//we don't care if we're overriding or creating a new field
			instance->table.set(READ_STRING(), peek(0));

			Value value = pop();
			pop();
			push(value);
			break;
		}
		case OP_SET_PROPERTY_LONG: {
			if (!IS_INSTANCE(peek(1))) {
				runtimeError("Only instances have fields.");
				return RUNTIME_ERROR;
			}

			objInstance* instance = AS_INSTANCE(peek(1));
			//we don't care if we're overriding or creating a new field
			instance->table.set(READ_STRING_LONG(), peek(0));

			Value value = pop();
			pop();
			push(value);
			break;
		}

		case OP_CREATE_STRUCT: {
			int numOfFields = READ_BYTE();

			//passing null instead of class signals to the VM that this is a struct, and not a instance of a class
			objInstance* inst = new objInstance(nullptr);

			//the compiler emits the fields in reverse order, so we can loop through them normally and pop the values on the stack
			for (int i = 0; i < numOfFields; i++) {
				objString* name = READ_STRING();
				inst->table.set(name, pop());
			}
			push(OBJ_VAL(inst));
			break;
		}
		case OP_CREATE_STRUCT_LONG: {
			int numOfFields = READ_BYTE();

			//passing null instead of class signals to the VM that this is a struct, and not a instance of a class
			objInstance* inst = new objInstance(nullptr);

			//the compiler emits the fields in reverse order, so we can loop through them normally and pop the values on the stack
			for (int i = 0; i < numOfFields; i++) {
				objString* name = READ_STRING_LONG();
				inst->table.set(name, pop());
			}
			push(OBJ_VAL(inst));
			break;
		}

		case OP_METHOD: {
			//class that this method binds too
			defineMethod(READ_STRING_LONG());
			break;
		}

		case OP_INVOKE: {
			//gets the method and calls it immediatelly, without converting it to a objBoundMethod
			objString* method = READ_STRING();
			int argCount = READ_BYTE();
			if (!invoke(method, argCount)) {
				return RUNTIME_ERROR;
			}
			frame = &frames[frameCount - 1];
			break;
		}
		case OP_INVOKE_LONG: {
			//gets the method and calls it immediatelly, without converting it to a objBoundMethod
			objString* method = READ_STRING_LONG();
			int argCount = READ_BYTE();
			if (!invoke(method, argCount)) {
				return RUNTIME_ERROR;
			}
			frame = &frames[frameCount - 1];
			break;
		}

		case OP_INHERIT: {
			Value superclass = peek(1);
			if (!IS_CLASS(superclass)) {
				runtimeError("Superclass must be a class.");
				return RUNTIME_ERROR;
			}
			objClass* subclass = AS_CLASS(peek(0));
			//copy down inheritance
			subclass->methods.tableAddAll(&subclass->methods);
			break;
		}

		case OP_GET_SUPER:{
			//super is ALWAYS followed by a field and is a call expr
			objString* name = READ_STRING();
			objClass* superclass = AS_CLASS(pop());

			if (!bindMethod(superclass, name)) {
				return RUNTIME_ERROR;
			}
			break;
		}
		case OP_GET_SUPER_LONG: {
			//super is ALWAYS followed by a field and is a call expr
			objString* name = READ_STRING_LONG();
			objClass* superclass = AS_CLASS(pop());

			if (!bindMethod(superclass, name)) {
				return RUNTIME_ERROR;
			}
			break;
		}

		case OP_SUPER_INVOKE: {
			//works same as OP_INVOKE, but uses invokeFromClass() to specify the superclass
			objString* method = READ_STRING();
			int argCount = READ_BYTE();
			objClass* superclass = AS_CLASS(pop());

			if (!invokeFromClass(superclass, method, argCount)) {
				return RUNTIME_ERROR;
			}
			frame = &frames[frameCount - 1];
			break;
		}
		case OP_SUPER_INVOKE_LONG: {
			//works same as OP_INVOKE, but uses invokeFromClass() to specify the superclass
			objString* method = READ_STRING_LONG();
			int argCount = READ_BYTE();
			objClass* superclass = AS_CLASS(pop());

			if (!invokeFromClass(superclass, method, argCount)) {
				return RUNTIME_ERROR;
			}
			frame = &frames[frameCount - 1];
			break;
		}
		#pragma endregion
		}
	}

	#undef READ_BYTE
	#undef READ_CONSTANT
	#undef BINARY_OP
	#undef INT_BINARY_OP
	#undef READ_STRING
	#undef READ_SHORT
}