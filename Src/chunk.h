#ifndef __IFS_CHUNK
	#define __IFS_CHUNK
	#include "common.h"

	using std::vector;

	enum OpCode {
		OP_CONSTANT,
		OP_RETURN
	};

	typedef double Value;

	class Chunk {
	public:
		vector<int> lines;
		vector<uint8_t> code;
		vector<Value> constants;
		Chunk();
		void writeData(uint8_t opCode, int line);
		void disassemble(string name);
		int addConstant(Value val);
	};

#endif // !__IFS_CHUNK
