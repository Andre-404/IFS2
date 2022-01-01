#ifndef __IFS_CHUNK
	#define __IFS_CHUNK
	#include "common.h"
	#include "value.h"

	using std::vector;

	enum OpCode {
		//Helpers
		OP_POP,
		OP_POPN,
		//constants
		OP_CONSTANT,
		OP_NIL,
		OP_TRUE,
		OP_FALSE,
		//unary
		OP_NEGATE,
		OP_NOT,
		OP_BIN_NOT,
		//binary
		OP_ADD,
		OP_SUBTRACT,
		OP_MULTIPLY,
		OP_DIVIDE,
		OP_MOD,
		OP_BITSHIFT_LEFT,
		OP_BITSHIFT_RIGHT,
		//comparisons and equality
		OP_EQUAL,
		OP_NOT_EQUAL,
		OP_GREATER,
		OP_GREATER_EQUAL,
		OP_LESS,
		OP_LESS_EQUAL,
		//Statements
		OP_PRINT,
		//Variables
		OP_DEFINE_GLOBAL,
		OP_GET_GLOBAL,
		OP_SET_GLOBAL,
		OP_GET_LOCAL,
		OP_SET_LOCAL,
		OP_RETURN
	};

	//disassebmle is here, but the functions it calls are in debug.cpp
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
