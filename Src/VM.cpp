#include "VM.h"
#include "debug.h"

vm::vm() {
	chunk = NULL;
	ip = 0;
}

vm::~vm() {

}

uint8_t vm::getOp(long _ip) {
	return chunk->code[_ip];
}

interpretResult vm::interpret(Chunk* _chunk) {
	chunk = _chunk;
	ip = 0;
	return run();
}

interpretResult vm::run() {
	#define READ_BYTE() (getOp(ip++))
	#define READ_CONSTANT() (chunk->constants[READ_BYTE()])

	while (true) {
		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				printValue(constant);
				printf("\n");
				break;
			}
			case OP_RETURN: {
				return INTERPRETER_OK;
			}
		}
	}

	#undef READ_BYTE
	#undef READ_CONSTANT
}