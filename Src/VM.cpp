#include "VM.h"
#include "debug.h"
#include <stdarg.h>

vm::vm(compiler* current) {
	curChunk = NULL;
	ip = 0;
	resetStack();
	if (!current->compiled) return;
	interpret(current->getCurrent());
}

vm::~vm() {

}

#pragma region Helpers

uint8_t vm::getOp(long _ip) {
	return curChunk->code[_ip];
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

void vm::runtimeError(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction = ip - 1;
	int line = curChunk->lines[instruction];
	fprintf(stderr, "[line %d] in script\n", line);
	resetStack();
}

static bool isFalsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
#pragma endregion



interpretResult vm::interpret(chunk* _chunk) {
	curChunk = _chunk;
	ip = 0;
	return run();
}

interpretResult vm::run() {
	#define READ_BYTE() (getOp(ip++))
	#define READ_CONSTANT() (curChunk->constants[READ_BYTE()])
	#define BINARY_OP(valueType, op) \
		do { \
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return INTERPRETER_RUNTIME_ERROR; \
			} \
			double b = AS_NUMBER(pop()); \
			double a = AS_NUMBER(pop()); \
			push(valueType(a op b)); \
		} while (false)
	#define INT_BINARY_OP(valueType, op)\
		do {\
			if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return INTERPRETER_RUNTIME_ERROR; \
			} \
			double b = AS_NUMBER(pop()); \
			double a = AS_NUMBER(pop()); \
			push(valueType((double)((int)a op (int)b))); \
		} while (false)

	while (true) {
		#ifdef DEBUG_TRACE_EXECUTION
		std::cout << "          ";
		for (Value* slot = stack; slot < stackTop; slot++) {
			std::cout << "[";
			printValue(*slot);
			std::cout << "] ";
		}
		std::cout << "\n";
		disassembleInstruction(curChunk, ip);
		#endif

		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
				break;
			}
			case OP_NIL: push(NIL_VAL()); break;
			case OP_TRUE: push(BOOL_VAL(true)); break;
			case OP_FALSE: push(BOOL_VAL(false)); break;
			case OP_NEGATE:
				if (!IS_NUMBER(peek(0))) {
					runtimeError("Operand must be a number.");
					return INTERPRETER_RUNTIME_ERROR;
				}
				push(NUMBER_VAL(-AS_NUMBER(pop())));
				break;
			case OP_NOT:
				push(BOOL_VAL(isFalsey(pop())));
				break;
			case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
			case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
			case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
			case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
			case OP_MOD:	  INT_BINARY_OP(NUMBER_VAL, %); break;
			case OP_BITSHIFT_LEFT: INT_BINARY_OP(NUMBER_VAL, <<); break;
			case OP_BITSHIFT_RIGHT: INT_BINARY_OP(NUMBER_VAL, >>); break;
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
			case OP_RETURN: {
				printValue(pop());
				std::cout << "\n";
				return INTERPRETER_OK;
			}
		}
	}

	#undef READ_BYTE
	#undef READ_CONSTANT
	#undef BINARY_OP
}