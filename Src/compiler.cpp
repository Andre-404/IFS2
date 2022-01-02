#include "compiler.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

compiler::compiler(parser* Parser) {
	compiled = true;
	current = NULL;
	line = 0;
	//locals
	scopeDepth = 0;
	localCount = 0;
	//this list will be passed to the vm when it's created
	objects = NULL;
	//Don't compile if we had a parse error
	if (Parser->hadError) {
		//Do nothing
		compiled = false;
	}
	else {
		current = new chunk;
		try {
			for (int i = 0; i < Parser->statements.size(); i++) {
				Parser->statements[i]->accept(this);
			}
			emitReturn();
			#ifdef DEBUG_PRINT_CODE
						getCurrent()->disassemble("code");
			#endif
		}
		catch (int e) {
			delete current;
			current = NULL;
			compiled = false;
		}
	}
}

compiler::~compiler() {

}

void error(string message);

void compiler::visitAssignmentExpr(ASTAssignmentExpr* expr) {
	expr->getVal()->accept(this);//compile the right side of the expression
	namedVar(expr->getToken(), true);
}

void compiler::visitOrExpr(ASTOrExpr* expr) {
	expr->getLeft()->accept(this);
	//if the left side is true, we know that the whole expression will eval to true
	int jump = emitJump(OP_JUMP_IF_TRUE);
	//if now we pop the left side and eval the right side, right side result becomes the result of the whole expression
	emitByte(OP_POP);
	expr->getRight()->accept(this);
	patchJump(jump);
}

void compiler::visitAndExpr(ASTAndExpr* expr) {
	expr->getLeft()->accept(this);
	//at this point we have the left side of the expression on the stack, and if it's false we skip to the end
	//since we know the whole expression will evaluate to false
	int jump = emitJump(OP_JUMP_IF_FALSE);
	//if the left side is true, we pop it and then push the right side to the stack, and the result of right side becomes the result of
	//whole expression
	emitByte(OP_POP);
	expr->getRight()->accept(this);
	patchJump(jump);
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
		namedVar(token, false);
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
	//if this is a global, we get a string constant index, if it's a local, it returns a dummy 0
	line = stmt->getToken().line;
	uint8_t global = parseVar(stmt->getToken());
	//compile the right side of the declaration, if there is no right side, the variable is initialized as nil
	ASTNode* expr = stmt->getExpr();
	if (expr == NULL) {
		emitByte(OP_NIL);
	}else{
		expr->accept(this);
	}
	//if this is a global var, we emit the code to define it in the VMs global hash table, if it's a local, do nothing
	//the slot that the compiled value is at becomes a local var
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

void compiler::visitBlockStmt(ASTBlockStmt* stmt) {
	beginScope();
	vector<ASTNode*> stmts = stmt->getStmts();
	for (ASTNode* node : stmts) {
		node->accept(this);
	}
	endScope();
}

void compiler::visitIfStmt(ASTIfStmt* stmt) {
	//compile condition and emit a jump over then branch if the condition is false
	stmt->getCondition()->accept(this);
	int thenJump = emitJump(OP_JUMP_IF_FALSE_POP);
	stmt->getThen()->accept(this);
	//only compile if there is a else branch
	if (stmt->getElse() != NULL) {
		//prevents fallthrough to else branch
		int elseJump = emitJump(OP_JUMP);
		patchJump(thenJump);
		
		stmt->getElse()->accept(this);
		patchJump(elseJump);
	}else patchJump(thenJump);

}

void compiler::visitWhileStmt(ASTWhileStmt* stmt) {
	//the bytecode for this is almost the same as if statement
	//but at the end of the body, we loop back to the start of the condition
	int loopStart = current->code.size();
	stmt->getCondition()->accept(this);
	int jump = emitJump(OP_JUMP_IF_FALSE_POP);
	stmt->getBody()->accept(this);
	emitLoop(loopStart);
	patchJump(jump);
	patchBreak();
}

void compiler::visitForStmt(ASTForStmt* stmt) {
	//we wrap this in a scope so if there is a var declaration in the initialization it's scoped to the loop
	beginScope();
	if(stmt->getInit() != NULL) stmt->getInit()->accept(this);
	int loopStart = current->code.size();
	//only emit the exit jump code if there is a condition expression
	int exitJump = -1;
	if (stmt->getCondition() != NULL) { 
		stmt->getCondition()->accept(this); 
		exitJump = emitJump(OP_JUMP_IF_FALSE_POP);
	}
	//body is mandatory
	stmt->getBody()->accept(this);
	//if there is a increment expression, we compile it and emit a POP to get rid of the result
	if (stmt->getIncrement() != NULL) {
		stmt->getIncrement()->accept(this);
		emitByte(OP_POP);
	}
	emitLoop(loopStart);
	//exit jump still needs to handle scoping appropriately 
	if (exitJump != -1) patchJump(exitJump);
	endScope();
	//patch breaks AFTER we close the for scope
	patchBreak();
}

void compiler::visitBreakStmt(ASTBreakStmt* stmt) {
	//the amount of variables to pop and the amount of code to jump is determined in handleBreak()
	//which is called at the end of loops
	line = stmt->getToken().line;
	emitByte(OP_BREAK);
	int breakJump = current->code.size();
	emitBytes(0xff, 0xff);
	emitBytes(0xff, 0xff);
	breakStmts.emplace_back(scopeDepth, breakJump, localCount);
}

#pragma region helpers

#pragma region Emitting bytes

void compiler::emitByte(uint8_t byte) {
	current->writeData(byte, line);//line is incremented whenever we find a statement/expression that contains tokens
}

void compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
	emitByte(byte1);
	emitByte(byte2);
}

void compiler::emit16Bit(unsigned short number) {
	emitBytes((number >> 8) & 0xff, number & 0xff);
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

int compiler::emitJump(OpCode jumpType) {
	emitByte((uint8_t)jumpType);
	emitBytes(0xff, 0xff);
	return current->code.size() - 2;
}

void compiler::patchJump(int offset) {
	// -2 to adjust for the bytecode for the jump offset itself.
	int jump = current->code.size() - offset - 2;
	//fix for future: insert 2 more bytes into the array, but make sure to do the same in lines array
	if (jump > UINT16_MAX) {
		error("Too much code to jump over.");
	}
	current->code[offset] = (jump >> 8) & 0xff;
	current->code[offset + 1] = jump & 0xff;
}

void compiler::emitLoop(int start) {
	emitByte(OP_LOOP);

	int offset = current->code.size() - start + 2;
	if (offset > UINT16_MAX) error("Loop body too large.");

	emit16Bit(offset);
}

#pragma endregion

#pragma region Variables

bool identifiersEqual(Token* a, Token* b) {
	return (a->lexeme.size() == b->lexeme.size() && a->lexeme.compare(b->lexeme) == 0);
}

uint8_t compiler::identifierConstant(Token name) {
	string temp(name.lexeme);
	return makeConstant(OBJ_VAL(copyString(temp)));
}

void compiler::defineVar(uint8_t name) {
	//if this is a local var, bail out
	if (scopeDepth > 0) { 
		markInit();
		return; 
	}
	emitBytes(OP_DEFINE_GLOBAL, name);
}

void compiler::namedVar(Token token, bool canAssign) {
	uint8_t getOp;
	uint8_t setOp;
	int arg = resolveLocal(token);
	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	}else {
		arg = identifierConstant(token);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}
	emitBytes(canAssign ? setOp : getOp, arg);
}

uint8_t compiler::parseVar(Token name) {
	declareVar(name);
	if (scopeDepth > 0) return 0;
	return identifierConstant(name);
}

void compiler::declareVar(Token& name) {
	if (scopeDepth == 0) return;
	for (int i = localCount - 1; i >= 0; i--) {
		local* _local = &locals[i];
		if (_local->depth != -1 && _local->depth < scopeDepth) {
			break;
		}

		if (identifiersEqual(&name, &_local->name)) {
			error("Already a variable with this name in this scope.");
		}
	}
	addLocal(name);
}

void compiler::addLocal(Token name) {
	if (localCount == LOCAL_MAX) {
		error("Too many local variables in function.");
		return;
	}
	local* _local = &locals[localCount++];
	_local->name = name;
	_local->depth = -1;
}

void compiler::endScope() {
	//Pop every variable that was declared in this scope
	scopeDepth--;//first lower the scope, the check for every var that is deeper than the current scope
	int toPop = 0;
	while (localCount > 0 && locals[localCount - 1].depth > scopeDepth) {
		toPop++;
		localCount--;
	}
	emitBytes(OP_POPN, toPop);
}

int compiler::resolveLocal(Token& name) {
	//checks to see if there is a local variable with a provided name, if there is return the index of the stack slot of the var
	for (int i = localCount - 1; i >= 0; i--) {
		local* _local = &locals[i];
		if (identifiersEqual(&name, &_local->name)) {
			if (_local->depth == -1) {
				error("Can't read local variable in its own initializer.");
			}
			return i;
		}
	}

	return -1;
}

void compiler::markInit() {
	//marks variable as ready to use, any use of it before this call is a error
	locals[localCount - 1].depth = scopeDepth;
}

void compiler::patchBreak() {
	int curCode = current->code.size();
	//most recent breaks are going to be on top
	for (int i = breakStmts.size() - 1; i >= 0; i--) {
		_break curBreak = breakStmts[i];
		//if the break stmt we're looking at has a depth that's equal or higher than current depth, we bail out
		if (curBreak.depth > scopeDepth) {
			int jumpLenght = curCode - curBreak.offset - 4;
			int toPop = curBreak.varNum - localCount;
			//variables declared by the time we hit the break whose depth is lower or equal to this break stmt
			current->code[curBreak.offset] = (toPop >> 8) & 0xff;
			current->code[curBreak.offset + 1] = toPop & 0xff;
			//amount to jump
			current->code[curBreak.offset + 2] = (jumpLenght >> 8) & 0xff;
			current->code[curBreak.offset + 3] = jumpLenght & 0xff;
			//delete break from array
			breakStmts.pop_back();
		}else break;
	}
}

#pragma endregion

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
