#include "debug.h"
#include "value.h"

debugASTPrinter::debugASTPrinter(vector<ASTNode*> _stmts) {
	str = "";
	inLocal = false;
	for (int i = 0; i < _stmts.size(); i++) {
		_stmts[i]->accept(this);
	}
}

void debugASTPrinter::buildExpr(std::string_view name, const std::initializer_list<ASTNode*> &exprs) {
	str.append("(").append(name);
	for (ASTNode* expr : exprs) {
		str.append(" ");
		expr->accept(this);
	}
	str.append(")");
}

void debugASTPrinter::visitAssignmentExpr(ASTAssignmentExpr* expr) {
	str.append("(").append(inLocal ? "Local " : "Global ").append(expr->getToken().lexeme);
	str.append(" = ");
	expr->getVal()->accept(this);
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

void debugASTPrinter::visitExprStmt(ASTExprStmt* stmt) {
	str.append("Expression stmt ");
	stmt->getExpr()->accept(this);
	str.append("\n");
	std::cout << str;
	str = "";
}

void debugASTPrinter::visitPrintStmt(ASTPrintStmt* stmt) {
	str.append("Print ");
	stmt->getExpr()->accept(this);
	str.append("\n");
	std::cout << str;
	str = "";
}

void debugASTPrinter::visitVarDecl(ASTVarDecl* stmt) {
	str.append(inLocal ? "Local " : "Global ");
	str.append("variable declaration for '");
	str.append(stmt->getToken().lexeme);
	str.append("' = ");
	stmt->getExpr()->accept(this);
	str.append("\n");
	std::cout << str;
	str = "";
}

void debugASTPrinter::visitBlockStmt(ASTBlockStmt* stmt) {
	inLocal = true;
	str.append("{\n");
	for (int i = 0; i < stmt->getStmts().size(); i++) {
		stmt->getStmts()[i]->accept(this);
	}
	str.append("}\n");
	std::cout << str;
	str = "";
	inLocal = false;
}

#pragma region Disassembly

static int simpleInstruction(string name, int offset) {
	std::cout << name << "\n";
	return offset + 1;
}

static int byteInstruction(const char* name, chunk* Chunk, int offset) {
	uint8_t slot = Chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
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
	case OP_POP:
		return simpleInstruction("OP POP", offset);
	case OP_POPN:
		return byteInstruction("OP POPN", Chunk, offset);
	case OP_PRINT:
		return simpleInstruction("OP PRINT", offset);
	case OP_DEFINE_GLOBAL:
		return constantInstruction("OP DEFINE GLOBAL", Chunk, offset);
	case OP_GET_GLOBAL:
		return constantInstruction("OP GET GLOBAL", Chunk, offset);
	case OP_SET_GLOBAL:
		return constantInstruction("OP SET GLOBAL", Chunk, offset);
	case OP_GET_LOCAL:
		return byteInstruction("OP_GET_LOCAL", Chunk, offset);
	case OP_SET_LOCAL:
		return byteInstruction("OP_SET_LOCAL", Chunk, offset);
	default:
		std::cout << "Unknown opcode " << (int)instruction << "\n";
		return offset + 1;
	}
}

#pragma endregion
