#include "compiler.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

compiler::compiler(parser* Parser) {
	compiled = true;
	current = NULL;
	line = 0;
	//this list will be passed to the vm when it's created
	objects = NULL;
	//Don't compile if we had a parse error
	if (Parser->hadError) {
		//Do nothing
		compiled = false;
	}
	else {
		current = new chunk;
		for (int i = 0; i < Parser->statements.size(); i++) {
			Parser->statements[i]->accept(this);
		}
		emitReturn();
	#ifdef DEBUG_PRINT_CODE
		getCurrent()->disassemble("code");
	#endif
	}
}

compiler::~compiler() {

}

void error(string message);

void compiler::visitAssignmentExpr(ASTAssignmentExpr* expr) {
	expr->getVal()->accept(this);
	uint8_t global = identifierConstant(expr->getToken());
	emitBytes(OP_SET_GLOBAL, global);
}

void compiler::visitBinaryExpr(ASTBinaryExpr* expr) {
	expr->getLeft()->accept(this);
	expr->getRight()->accept(this);
	line = expr->getToken().line;
	switch (expr->getToken().type) {
	//take in double or string(in case of add)
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
	const Token& token = expr->getToken();
	line = token.line;
	switch (token.type) {
	case TOKEN_NUMBER: {
		double num = std::stod(string(token.lexeme));//doing this becuase stod doesn't accept string_view
		emitConstant(NUMBER_VAL(num));
		break;
	}
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_NIL: emitByte(OP_NIL); break;
	case TOKEN_STRING: {
		//this gets rid of quotes, eg. ""Hello world""->"Hello world"
		string _temp(token.lexeme);//converts string_view to string
		_temp.erase(0, 1);
		_temp.erase(_temp.size() - 1, 1);
		emitConstant(OBJ_VAL(appendObject(copyString(_temp))));
		break;
	}

	case TOKEN_IDENTIFIER: {
		namedVar(token);
		break;
	}
	}
}

void compiler::visitUnaryExpr(ASTUnaryExpr* expr) {
	expr->getRight()->accept(this);
	Token token = expr->getToken();
	line = token.line;
	switch (token.type) {
	case TOKEN_MINUS: emitByte(OP_NEGATE); break;
	case TOKEN_BANG: emitByte(OP_NOT); break;
	case TOKEN_TILDA: emitByte(OP_BIN_NOT); break;
	}
}

void compiler::visitVarDecl(ASTVarDecl* stmt) {
	uint8_t global = identifierConstant(stmt->getToken());
	ASTNode* expr = stmt->getExpr();
	if (expr == NULL) {
		emitByte(OP_NIL);
	}else{
		expr->accept(this);
	}
	defineVar(global);
}

void compiler::visitPrintStmt(ASTPrintStmt* stmt) {
	stmt->getExpr()->accept(this);
	emitByte(OP_PRINT);
}

void compiler::visitExprStmt(ASTExprStmt* stmt) {
	stmt->getExpr()->accept(this);
	emitByte(OP_POP);
}


#pragma region helpers
void compiler::emitByte(uint8_t byte) {
	current->writeData(byte, line);//line is incremented whenever we find a statement/expression that contains tokens
}

void compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
	emitByte(byte1);
	emitByte(byte2);
}

uint8_t compiler::makeConstant(Value value) {
	int constant = getCurrent()->addConstant(value);
	if (constant > UINT8_MAX) {
		error("Too many constants in one chunk.");
		return 0;
	}

	return (uint8_t)constant;
}

void compiler::emitConstant(Value value) {
	//shorthand for adding a constant to the chunk and emitting it
	emitBytes(OP_CONSTANT, makeConstant(value));
}


void compiler::emitReturn() {
	emitByte(OP_RETURN);
}

uint8_t compiler::identifierConstant(Token name) {
	string temp(name.lexeme);
	return makeConstant(OBJ_VAL(copyString(temp)));
}

void compiler::defineVar(uint8_t name) {
	emitBytes(OP_DEFINE_GLOBAL, name);
}

void compiler::namedVar(Token token) {
	uint8_t arg = identifierConstant(token);
	emitBytes(OP_GET_GLOBAL, arg);
}

chunk* compiler::getCurrent() {
	return current;
}

void error(string message) {
	std::cout << "Compile error: " << message << "\n";
	throw 20;
}
//adds the object to compilers objects list
obj* compiler::appendObject(obj* _object) {
	_object->next = objects;
	objects = _object;
	return _object;
}

#pragma endregion
