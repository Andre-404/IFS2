#include "compiler.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

compiler::compiler(parser* Parser) {
	compiled = true;
	current = NULL;
	line = 0;
	if (Parser->hadError) {
		//Do nothing
		compiled = false;
	}
	else {
		current = new chunk;
		try {
			Parser->tree->accept(this);
			emitReturn();
			#ifdef DEBUG_PRINT_CODE
						getCurrent()->disassemble("code");
			#endif
		}
		catch (int e) {
			//Do nothing
		}
	}
}

compiler::~compiler() {

}

void error(string message);

void compiler::visitBinaryExpr(ASTBinaryExpr* expr) {
	expr->getLeft()->accept(this);
	expr->getRight()->accept(this);
	line = expr->getToken().line;
	switch (expr->getToken().type) {
	case TOKEN_PLUS: emitByte(OP_ADD); break;
	case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
	case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
	case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
	//operands get cast to int for these ops
	case TOKEN_PERCENTAGE: emitByte(OP_MOD); break;
	case TOKEN_BITSHIFT_LEFT: emitByte(OP_BITSHIFT_LEFT); break;
	case TOKEN_BITSHIFT_RIGHT: emitByte(OP_BITSHIFT_RIGHT); break;
	//these return bools
	case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
	case TOKEN_BANG_EQUAL: emitByte(OP_NOT_EQUAL); break;
	case TOKEN_GREATER: emitByte(OP_GREATER); break;
	case TOKEN_GREATER_EQUAL: emitByte(OP_GREATER_EQUAL); break;
	case TOKEN_LESS: emitByte(OP_LESS); break;
	case TOKEN_LESS_EQUAL: emitByte(OP_LESS_EQUAL); break;
	}
}

void compiler::visitGroupingExpr(ASTGroupingExpr* expr) {
	expr->getExpr()->accept(this);
}

void compiler::visitLiteralExpr(ASTLiteralExpr* expr) {
	Token token = expr->getToken();
	line = token.line;
	switch (token.type) {
	case TOKEN_NUMBER: {
		double g = std::stod(token.lexeme);
		Value val = Value();
		int constant = current->addConstant(NUMBER_VAL(g));
		if (constant > 256) {
			compiled = false;
		}
		emitBytes(OP_CONSTANT, constant);
		break; 
	}
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_NIL: emitByte(OP_NIL); break;
	}
}

void compiler::visitUnaryExpr(ASTUnaryExpr* expr) {
	expr->getRight()->accept(this);
	Token token = expr->getToken();
	line = token.line;
	switch (token.type) {
	case TOKEN_MINUS: emitByte(OP_NEGATE); break;
	case TOKEN_BANG: emitByte(OP_NOT); break;
	}
}


#pragma region helpers
void compiler::emitByte(uint8_t byte) {
	current->writeData(byte, line);
}

void compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
	emitByte(byte1);
	emitByte(byte2);
}

void compiler::emitReturn() {
	emitByte(OP_RETURN);
}

chunk* compiler::getCurrent() {
	return current;
}

void error(string message) {
	std::cout << "Compile error: " << message << "\n";
	throw 20;
}

#pragma endregion
