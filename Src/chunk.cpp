#include "chunk.h"
#include "debug.h"

Chunk::Chunk(){

}

void Chunk::writeData(uint8_t opCode, int line) {
	code.push_back(opCode);
	lines.push_back(line);
}

int Chunk::addConstant(Value val) {
	int size = constants.size();
	constants.push_back(val);
	return size;
}

void Chunk::disassemble(string name) {
	std::cout << "=======" << name << "=======\n";

	for (int offset = 0; offset < code.size();) {
		offset = disassembleInstruction(this, offset);
	}
}
