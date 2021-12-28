#include "debug.h"
#include "value.h"

debugASTPrinter::debugASTPrinter(ASTNode* expr) {
	str = "";
	expr->accept(this);
	std::cout << str<<"\n";
}

void debugASTPrinter::buildExpr(std::string_view name, const std::initializer_list<ASTNode*> &exprs) {
	str.append("(").append(name);
	for (ASTNode* expr : exprs) {
		str.append(" ");
		expr->accept(this);
	}
	str.append(")");
}

void debugASTPrinter::visitBinaryExpr(ASTBinaryExpr* expr) {
	buildExpr(expr->getToken().lexeme, { expr->getLeft(), expr->getRight() });
}
void debugASTPrinter::visitUnaryExpr(ASTUnaryExpr* expr) {
	buildExpr(expr->getToken().lexeme, { expr->getRight() });
}
void debugASTPrinter::visitGroupingExpr(ASTGroupingExpr* expr) {
	buildExpr("group", { expr->getExpr() });
}
void debugASTPrinter::visitLiteralExpr(ASTLiteralExpr* expr) {
	str.append(expr->getToken().lexeme);
}

#pragma region Disassembly

static int simpleInstruction(string name, int offset) {
	std::cout << name << "\n";
	return offset + 1;
}

static int constantInstruction(string name, chunk* Chunk, int offset) {
	uint8_t constant = Chunk->code[offset + 1];
	printf("%-16s %4d '", name.c_str(), constant);//have to use printf because of string spacing
	printValue(Chunk->constants[constant]);
	printf("'\n");
	return offset + 2;
}

int disassembleInstruction(chunk* Chunk, int offset) {
	//printf usage because of %04d
	printf("%04d ", offset);

	if (offset > 0 && Chunk->lines[offset] == Chunk->lines[offset - 1]) {
		std::cout << "   | ";
	}else {
		printf("%4d ", Chunk->lines[offset]);
	}

	uint8_t instruction = Chunk->code[offset];
	switch (instruction) {
	case OP_CONSTANT:
		return constantInstruction("OP_CONSTANT", Chunk, offset);
	case OP_NIL:
		return simpleInstruction("OP_NIL", offset);
	case OP_TRUE:
		return simpleInstruction("OP_TRUE", offset);
	case OP_FALSE:
		return simpleInstruction("OP_FALSE", offset);
	case OP_NEGATE:
		return simpleInstruction("OP_NEGATE", offset);
	case OP_ADD:
		return simpleInstruction("OP_ADD", offset);
	case OP_SUBTRACT:
		return simpleInstruction("OP_SUBTRACT", offset);
	case OP_MULTIPLY:
		return simpleInstruction("OP_MULTIPLY", offset);
	case OP_DIVIDE:
		return simpleInstruction("OP_DIVIDE", offset);
	case OP_MOD:
		return simpleInstruction("OP_MOD", offset);
	case OP_BITSHIFT_LEFT:
		return simpleInstruction("OP_BITSHIFT_LEFT", offset);
	case OP_BITSHIFT_RIGHT:
		return simpleInstruction("OP_BITSHIFT_RIGHT", offset);
	case OP_RETURN:
		return simpleInstruction("OP_RETURN", offset);
	case OP_NOT:
		return simpleInstruction("OP_NOT", offset);
	case OP_EQUAL:
		return simpleInstruction("OP_EQUAL", offset);
	case OP_NOT_EQUAL:
		return simpleInstruction("OP NOT EQUAL", offset);
	case OP_GREATER:
		return simpleInstruction("OP_GREATER", offset);
	case OP_GREATER_EQUAL:
		return simpleInstruction("OP GREATER EQUAL", offset);
	case OP_LESS:
		return simpleInstruction("OP_LESS", offset);
	case OP_LESS_EQUAL: 
		return simpleInstruction("OP LESS EQUAL", offset);
	default:
		std::cout << "Unknown opcode " << (int)instruction << "\n";
		return offset + 1;
	}
}

#pragma endregion
