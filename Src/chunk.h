#pragma once

#include "common.h"
#include "value.h"
#include "switch.h"
#include "gcVector.h"

using std::vector;

enum OpCode {
	//Helpers
	OP_POP,
	OP_POPN,
	//constants
	OP_CONSTANT,
	OP_CONSTANT_LONG,
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
	OP_ADD_1,
	OP_SUBTRACT_1,
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
	OP_DEFINE_GLOBAL_LONG,
	OP_GET_GLOBAL,
	OP_GET_GLOBAL_LONG,
	OP_SET_GLOBAL,
	OP_SET_GLOBAL_LONG,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_GET_UPVALUE,
	OP_SET_UPVALUE,
	OP_CLOSE_UPVALUE,
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
	OP_CLOSURE_LONG,

	//OOP
	OP_CLASS,
	OP_GET_PROPERTY,
	OP_GET_PROPERTY_LONG,
	OP_SET_PROPERTY,
	OP_SET_PROPERTY_LONG,
	OP_CREATE_STRUCT,
	OP_CREATE_STRUCT_LONG,
	OP_METHOD,
	OP_INVOKE,
	OP_INVOKE_LONG,
	OP_INHERIT,
	OP_GET_SUPER,
	OP_GET_SUPER_LONG,
	OP_SUPER_INVOKE,
	OP_SUPER_INVOKE_LONG,

	//fibers
	OP_FIBER_CREATE,
	OP_FIBER_RUN,
	OP_FIBER_YIELD,
};

//disassemble is here, but the functions it calls are in debug.cpp
class chunk {
public:
	gcVector<int> lines;
	gcVector<uint8_t> code;
	gcVector<Value> constants;
	vector<switchTable> switchTables;
	chunk() {};
	void writeData(uint8_t opCode, int line);
	void disassemble(string name);
	uInt addConstant(Value val);
	int addSwitch(switchTable table);
};
