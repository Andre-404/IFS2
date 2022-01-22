#include "VM.h"
#include "debug.h"
#include "namespaces.h"
#include "builtInFunction.h"
#include <stdarg.h>

using namespace global;

vm::vm(compiler* current) {
	frameCount = 0;
	resetStack();
	if (!current->compiled) return;
	//we do this here because defineNative() mutates the globals field,
	//and if a collection happens we need to update all pointers in globals
	global::gc.VM = this;
	defineNative("clock", clockNative, 0);

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
	return interpretResult::INTERPRETER_RUNTIME_ERROR;
}

static bool isFalsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
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

void vm::freeObjects() {
	global::gc.clear();
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
	push(OBJ_VAL(copyString((char*)name.c_str(), name.length())));
	push(OBJ_VAL(new objNativeFn(func, arity)));
	globals.set(AS_STRING(stack[0]), stack[1]);
	pop();
	pop();
}

objUpval* vm::captureUpvalue(Value* local) {
	int i;
	for (i = openUpvals.size() - 1; i >= 0; i--) {
		if (openUpvals[i]->location >= local) {
			if (openUpvals[i]->location == local) return openUpvals[i];
		}
		else break;
	}
	objUpval* createdUpval = new objUpval(local);

	return createdUpval;
}

void vm::closeUpvalues(Value* last) {
	objUpval* upval;
	for (int i = openUpvals.size() - 1; i >= 0; i--) {
		upval = openUpvals[i];
		if (upval->location >= last) {
			upval->closed = *upval->location;
			upval->location = &upval->closed;
			upval->isOpen = false;
		}
		else break;
	}
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
	#define READ_CONSTANT() (frames[frameCount - 1].closure->func->body.constants[READ_BYTE()])
	#define READ_STRING() AS_STRING(READ_CONSTANT())
	#define BINARY_OP(valueType, op) \
		do { \
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return interpretResult::INTERPRETER_RUNTIME_ERROR; \
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

	#define READ_SHORT() (frame->ip += 2, (uint16_t)((getOp(frame->ip-2) << 8) | getOp(frame->ip-1)))

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
			push(NUMBER_VAL((double)~num));
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
		case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
		case OP_MOD:	  INT_BINARY_OP(NUMBER_VAL, %); break;
		case OP_BITSHIFT_LEFT: INT_BINARY_OP(NUMBER_VAL, <<); break;
		case OP_BITSHIFT_RIGHT: INT_BINARY_OP(NUMBER_VAL, >>); break;
		#pragma endregion

		#pragma region Binary that returns bools
		case OP_EQUAL: {
			Value b = pop();
			Value a = pop();
			push(BOOL_VAL(valuesEqual(a, b)));
			break;
		}
		case OP_NOT_EQUAL: BINARY_OP(BOOL_VAL, != ); break;
		case OP_GREATER: BINARY_OP(BOOL_VAL, > ); break;
		case OP_GREATER_EQUAL: BINARY_OP(BOOL_VAL, >= ); break;
		case OP_LESS: BINARY_OP(BOOL_VAL, < ); break;
		case OP_LESS_EQUAL: BINARY_OP(BOOL_VAL, <= ); break;
		#pragma endregion

		#pragma region Statements
		case OP_PRINT: {
			printValue(pop());
			std::cout << "\n";
			break;
		}

		case OP_DEFINE_GLOBAL: {
			objString* name = READ_STRING();
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

		case OP_SET_GLOBAL: {
			objString* name = READ_STRING();
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
		case OP_CLOSE_UPVALUE:
			closeUpvalues(stackTop - 1);
			pop();
			break;
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
				return interpretResult::INTERPRETER_RUNTIME_ERROR;
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
		#pragma endregion

		#pragma region Objects, arrays and maps
		case OP_CREATE_ARRAY: {
			int size = READ_BYTE();
			int i = 0;
			vector<Value> vals;
			while (i < size) {
				vals.push_back(peek(size - i - 1));
				i++;
			}
			objArray* arr = new objArray(vals);
			i = 0;
			while (i < size) {
				pop();
				i++;
			}
			push(OBJ_VAL(arr));
			break;
		}

		case OP_GET: {
			//structs and objects get their own OP_GET_DOT operator
			Value index = pop();
			Value callee = pop();
			if (!IS_OBJ(callee))
				runtimeError("Expected a array or map, got %s.", callee.type == VAL_NUM ? "number" : callee.type == VAL_NIL ? "nil" : "bool");
			switch (AS_OBJ(callee)->type) {
			case OBJ_ARRAY: {
				if (!IS_NUMBER(index)) return runtimeError("Index must be a number.");
				double ind = AS_NUMBER(index);
				if ((int)ind != ind) return runtimeError("Expected interger, got float.");
				if (ind < 0 || ind > AS_ARRAY(callee)->values.size() - 1)
					return runtimeError("Index %d outside of range [0, %d].", (int)ind, AS_ARRAY(callee)->values.size() - 1);

				push(AS_ARRAY(callee)->values[ind]);
				break;
			}
			}
			break;
		}

		case OP_SET: {
			//this is for handling arrays and maps, objects and structs get their own OP_SET_DOT operator
			Value val = peek(0);
			Value field = peek(1);
			Value callee = peek(2);
			if (!IS_OBJ(callee))
				runtimeError("Expected a array or map, got %s.", callee.type == VAL_NUM ? "number" : callee.type == VAL_NIL ? "nil" : "bool");
			switch (AS_OBJ(callee)->type) {
			case OBJ_ARRAY: {
				objArray* arr = AS_ARRAY(callee);

				if (!IS_NUMBER(field)) return runtimeError("Index has to be a number");
				double index = AS_NUMBER(field);
				if (index != (int)index) return runtimeError("Index has to be a integer.");
				if (index < 0 || index >arr->values.size() - 1)
					return runtimeError("Index %d outside of range [0, %d].", (int)index, arr->values.size() - 1);

				arr->values[index] = val;
				}
			}
			//we want only the value to remain on the stack, since set is a assignment expr
			pop();
			pop();
			pop();
			push(val);
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