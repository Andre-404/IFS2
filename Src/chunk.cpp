#include "chunk.h"
#include "debug.h"

void chunk::writeData(uint8_t opCode, uInt line, string& name) {
	code.push(opCode);
	if (lines.size() == 0) {
		lines.push_back(codeLine(line, name));
		return;
	}
	if (lines[lines.size() - 1].line == line) return;
	lines[lines.size() - 1].end = code.count();
	lines.push_back(codeLine(line, name));
}

//adds the constant to the array and returns it's index, which is used in conjuction with OP_CONSTANT
//first checks if this value already exists, this helps keep the constants array small
uInt chunk::addConstant(Value val) {
	for (uInt i = 0; i < constants.count(); i++) {
		if (valuesEqual(constants[i], val)) return i;
	}
	uInt size = constants.count();
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
	for (uInt offset = 0; offset < code.count();) {
		offset = disassembleInstruction(this, offset);
	}
}

codeLine chunk::getLine(uInt offset) {
	for (codeLine line : lines) {
		if (offset <= line.end) return line;
	}
	std::cout << "FUck";
	exit(64);
}
