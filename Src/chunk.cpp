#include "chunk.h"
#include "debug.h"

chunk::chunk(){

}


void chunk::writeData(uint8_t opCode, int line) {
	code.push_back(opCode);
	lines.push_back(line);
}

//adds the constant to the array and returns it's index, which is used in conjuction with OP_CONSTANT
int chunk::addConstant(Value val) {
	int size = constants.size();
	constants.push_back(val);
	return size;
}

void chunk::disassemble(string name) {
	std::cout << "=======" << name << "=======\n";
	//prints every instruction in chunk
	for (int offset = 0; offset < code.size();) {
		offset = disassembleInstruction(this, offset);
	}
}
