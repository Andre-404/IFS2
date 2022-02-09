#ifndef __IFS_CHUNK
	#define __IFS_CHUNK
	#include "common.h"
	#include "value.h"
	#include "switch.h"

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
		OP_BITWISE_XOR,
		OP_BITWISE_OR,
		OP_BITWISE_AND,
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
		OP_GET_UPVALUE,
		OP_SET_UPVALUE,
		OP_CLOSE_UPVALUE,
		OP_INCREMENT_PRE,
		OP_DECREMENT_PRE,
		OP_INCREMENT_POST,
		OP_DECREMENT_POST,
		//Arrays
		OP_CREATE_ARRAY,
		OP_GET,
		OP_SET,
		//control flow
		OP_JUMP,
		OP_JUMP_IF_FALSE,
		OP_JUMP_IF_TRUE,
		OP_JUMP_IF_FALSE_POP,
		OP_LOOP,
		OP_BREAK,
		OP_SWITCH,

		//Functions
		OP_CALL,
		OP_RETURN,
		OP_CLOSURE,

		//OOP
		OP_CLASS,
		OP_GET_PROPERTY,
		OP_SET_PROPERTY,
		OP_CREATE_STRUCT,
		OP_METHOD,
		OP_INVOKE,
		OP_INHERIT,
		OP_GET_SUPER,
		OP_SUPER_INVOKE,
		
		//Iterators
		OP_ITERATOR_START,
		OP_ITERATOR_GET,
		OP_ITERATOR_NEXT,
	};

	//disassemble is here, but the functions it calls are in debug.cpp
	class chunk {
	public:
		vector<int> lines;
		vector<uint8_t> code;
		vector<Value> constants;
		vector<switchTable> switchTables;
		chunk();
		void writeData(uint8_t opCode, int line);
		void disassemble(string name);
		int addConstant(Value val);
		int addSwitch(switchTable table);
	};

#endif // !__IFS_CHUNK
