#include "debug.h"
#include "value.h"
#include "object.h"

#ifdef DEBUG_PRINT_AST 

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
	if(stmt->getExpr() != NULL) stmt->getExpr()->accept(this);
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

void debugASTPrinter::visitIfStmt(ASTIfStmt* stmt) {
	str.append("if (");
	stmt->getCondition()->accept(this);
	str.append(")");
	stmt->getThen()->accept(this);
	str.append("else");
	if (stmt->getElse() != NULL) stmt->getElse()->accept(this);
}

void debugASTPrinter::visitWhileStmt(ASTWhileStmt* stmt) {
	str.append("while (");
	stmt->getCondition()->accept(this);
	str.append(")");
	stmt->getBody()->accept(this);
}

void debugASTPrinter::visitForStmt(ASTForStmt* stmt) {
	str.append("for (");
	stmt->getInit()->accept(this);
	stmt->getCondition()->accept(this);
	stmt->getIncrement()->accept(this);
	str.append(")");
	stmt->getBody()->accept(this);
}

void debugASTPrinter::visitBreakStmt(ASTBreakStmt* stmt) {
	str.append("Break\n");
	std::cout << str;
	str = "";
}

void debugASTPrinter::visitSwitchStmt(ASTSwitchStmt* stmt) {
	str.append("Switch( ");
	stmt->getExpr()->accept(this);
	str.append(" ){\n");
	for (ASTNode* _case : stmt->getCases()) {
		_case->accept(this);
	}
	str.append("}\n");
	std::cout << str;
	str = "";
}

void debugASTPrinter::visitCase(ASTCase* _case) {
	str.append("case ");
	if(_case->getExpr() != NULL)_case->getExpr()->accept(this);
	str.append(":\n");
	for (ASTNode* stmt : _case->getStmts()) {
		stmt->accept(this);
	}
	str.append("\n");
	std::cout << str;
	str = "";
}

void debugASTPrinter::visitFuncDecl(ASTFunc* decl) {

}

void debugASTPrinter::visitCallExpr(ASTCallExpr* expr) {

}

void debugASTPrinter::visitReturnStmt(ASTReturn* stmt) {

}

void debugASTPrinter::visitArrayDeclExpr(ASTArrayDeclExpr* expr) {

}

void debugASTPrinter::visitSetExpr(ASTSetExpr* expr) {

}
#endif // DEBUG_PRINT_AST

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

static int jumpInstruction(const char* name, int sign, chunk* Chunk, int offset) {
	uint16_t jump = (uint16_t)(Chunk->code[offset + 1] << 8);
	jump |= Chunk->code[offset + 2];
	printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
	return offset + 3;
}

static int invokeInstruction(const char* name, chunk* Chunk, int offset) {
	uint8_t constant = Chunk->code[offset + 1];
	uint8_t argCount = Chunk->code[offset + 2];
	printf("%-16s (%d args) %4d '", name, argCount, constant);
	printValue(Chunk->constants[constant]);
	printf("'\n");
	return offset + 3;
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
		return byteInstruction("OP GET LOCAL", Chunk, offset);
	case OP_SET_LOCAL:
		return byteInstruction("OP SET LOCAL", Chunk, offset);
	case OP_JUMP:
		return jumpInstruction("OP JUMP", 1, Chunk, offset);
	case OP_JUMP_IF_FALSE:
		return jumpInstruction("OP JUMP IF FALSE", 1, Chunk, offset);
	case OP_JUMP_IF_TRUE:
		return jumpInstruction("OP JUMP IF TRUE", 1, Chunk, offset);
	case OP_JUMP_IF_FALSE_POP:
		return jumpInstruction("OP JUMP IF FALSE POP", 1, Chunk, offset);
	case OP_LOOP:
		return jumpInstruction("OP LOOP", -1, Chunk, offset);
	case OP_BREAK: {
		uint16_t toPop = (uint16_t)(Chunk->code[offset + 1] << 8);
		toPop |= Chunk->code[offset + 2];
		uint16_t jump = (uint16_t)(Chunk->code[offset + 3] << 8);
		jump |= Chunk->code[offset + 4];
		printf("%-16s %4d -> %d POP %d\n", "OP BREAK" , offset, offset + 5 + jump, toPop);
		return offset + 5;
	}
	case OP_SWITCH:{
		return byteInstruction("OP SWITCH", Chunk, offset);
	}
	case OP_CALL:
		return byteInstruction("OP CALL", Chunk, offset);
	case OP_CREATE_ARRAY:
		return byteInstruction("OP CREATE ARRAY", Chunk, offset);
	case OP_GET:
		return simpleInstruction("OP GET", offset);
	case OP_SET:
		return byteInstruction("OP SET", Chunk, offset);
	case OP_CLOSURE: {
		offset++;
		uint8_t constant = Chunk->code[offset++];
		printf("%-16s %4d ", "OP CLOSURE", constant);
		printValue(Chunk->constants[constant]);
		printf("\n");

		objFunc* function = AS_FUNCTION(Chunk->constants[constant]);
		for (int j = 0; j < function->upvalueCount; j++) {
			int isLocal = Chunk->code[offset++];
			int index = Chunk->code[offset++];
			printf("%04d      |                     %s index: %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
		}
		return offset;
	}
	case OP_GET_UPVALUE:
		return byteInstruction("OP GET UPVALUE", Chunk, offset);
	case OP_SET_UPVALUE:
		return byteInstruction("OP SET UPVALUE", Chunk, offset);
	case OP_CLOSE_UPVALUE:
		return simpleInstruction("OP CLOSE UPVALUE", offset);
	case OP_CLASS:
		return constantInstruction("OP CLASS", Chunk, offset);
	case OP_GET_PROPERTY:
		return constantInstruction("OP GET PROPERTY", Chunk, offset);
	case OP_SET_PROPERTY:
		return constantInstruction("OP SET PROPERTY", Chunk, offset);
	case OP_CREATE_STRUCT: {
		offset++;
		uint8_t fieldNum = Chunk->code[offset++];
		printf("%-16s %4d ", "OP CREATE STRUCT", fieldNum);
		printf("\n");
		for (int i = 0; i < fieldNum; i++) {
			int constant = Chunk->code[offset++];
			printf("%04d    | %-16s %4d\n", offset-1, "FIELD CONSTANT", constant);
		}
		return offset;
	}
	case OP_METHOD:
		return constantInstruction("OP METHOD", Chunk, offset);
	case OP_INVOKE:
		return invokeInstruction("OP INVOKE", Chunk, offset);
	case OP_INHERIT:
		return simpleInstruction("OP INHERIT", offset);
	case OP_GET_SUPER:
		return constantInstruction("OP GET SUPER", Chunk, offset);
	case OP_SUPER_INVOKE:
		return invokeInstruction("OP SUPER INVOKE", Chunk, offset);
	default:
		std::cout << "Unknown opcode " << (int)instruction << "\n";
		return offset + 1;
	}
}

#pragma endregion
