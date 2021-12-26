#ifndef __IFS_CHUNK
	#define __IFS_CHUNK
	#include "common.h"
	#include "value.h"

	using std::vector;

	enum OpCode {
		OP_CONSTANT,
		OP_NIL,
		OP_TRUE,
		OP_FALSE,
		OP_NEGATE,
		OP_NOT,
		OP_ADD,
		OP_SUBTRACT,
		OP_MULTIPLY,
		OP_DIVIDE,
		OP_MOD,
		OP_BITSHIFT_LEFT,
		OP_BITSHIFT_RIGHT,
		OP_EQUAL,
		OP_NOT_EQUAL,
		OP_GREATER,
		OP_GREATER_EQUAL,
		OP_LESS,
		OP_LESS_EQUAL,
		OP_RETURN
	};


	class chunk {
	public:
		vector<int> lines;
		vector<uint8_t> code;
		vector<Value> constants;
		chunk();
		void writeData(uint8_t opCode, int line);
		void disassemble(string name);
		int addConstant(Value val);
	};

#endif // !__IFS_CHUNK
