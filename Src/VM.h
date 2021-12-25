#ifndef __IFS_VM
#define __IFS_VM

#include "common.h"
#include "chunk.h"

enum interpretResult {
	INTERPRETER_OK,
	INTERPRETER_RUNTIME_ERROR
};

class vm {
public:
	vm();
	~vm();
	uint8_t getOp(long _ip);
	interpretResult interpret(Chunk* _chunk);
	interpretResult run();
private:
	long ip;
	Chunk* chunk;
};


#endif // !__IFS_VM
