#include "chunk.h"
#include "debug.h"

chunk::chunk(){

}


void chunk::writeData(uint8_t opCode, int line) {
	code.push(opCode);
	lines.push(line);
}

//adds the constant to the array and returns it's index, which is used in conjuction with OP_CONSTANT
int chunk::addConstant(Value val) {
	int size = constants.count();
	constants.push(val);
	return size;
}

int chunk::addSwitch(switchTable table) {
	int size = switchTables.size();
	switchTables.push_back(table);
	return size;
}

void chunk::disassemble(string name) {
	std::cout << "=======" << name << "=======\n";
	//prints every instruction in chunk
	for (int offset = 0; offset < code.count();) {
		offset = disassembleInstruction(this, offset);
	}
}
