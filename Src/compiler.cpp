#include "compiler.h"
#include "namespaces.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

using namespace global;

compilerInfo::compilerInfo(compilerInfo* _enclosing, funcType _type) : enclosing(_enclosing), type(_type) {
	//first slot is claimed for function name
	local* _local = &locals[localCount++];
	_local->depth = 0;
	_local->name = "";
	func = new objFunc();
	code = &func->body;
}


compiler::compiler(parser* _parser, funcType _type) {
	Parser = _parser;
	compiled = true;
	global::gc.compiling = this;
	current = new compilerInfo(nullptr, funcType::TYPE_SCRIPT);
	//Don't compile if we had a parse error
	if (Parser->hadError) {
		//Do nothing
		compiled = false;
	}
	else {
		try {
			for (int i = 0; i < Parser->statements.size(); i++) {
				Parser->statements[i]->accept(this);
			}
		}
		catch (int e) {
			compiled = false;
		}
	}
}

compiler::~compiler() {
	delete Parser;
}

void error(string message);

void compiler::visitAssignmentExpr(ASTAssignmentExpr* expr) {
	expr->getVal()->accept(this);//compile the right side of the expression
	namedVar(expr->getToken(), true);
}

void compiler::visitSetExpr(ASTSetExpr* expr) {
	//different behaviour for '[' and '.'
	switch (expr->getAccessor().type) {
	case TOKEN_LEFT_BRACKET: {
		expr->getCallee()->accept(this);
		expr->getField()->accept(this);
		expr->getValue()->accept(this);
		emitByte(OP_SET);
		break;
	}
	//this is a optimization for when the identifier is already there
	case TOKEN_DOT: {
		expr->getCallee()->accept(this);
		expr->getValue()->accept(this);
		int name = identifierConstant(((ASTLiteralExpr*)expr->getField())->getToken());
		emitBytes(OP_SET_PROPERTY, name);
		break;
	}
	}
}

void compiler::visitBinaryExpr(ASTBinaryExpr* expr) {
	expr->getLeft()->accept(this);
	if (expr->getToken().type == TOKEN_OR) {
		//if the left side is true, we know that the whole expression will eval to true
		int jump = emitJump(OP_JUMP_IF_TRUE);
		//if now we pop the left side and eval the right side, right side result becomes the result of the whole expression
		emitByte(OP_POP);
		expr->getRight()->accept(this);
		patchJump(jump);
		return;
	}else if (expr->getToken().type == TOKEN_AND) {
		//at this point we have the left side of the expression on the stack, and if it's false we skip to the end
		//since we know the whole expression will evaluate to false
		int jump = emitJump(OP_JUMP_IF_FALSE);
		//if the left side is true, we pop it and then push the right side to the stack, and the result of right side becomes the result of
		//whole expression
		emitByte(OP_POP);
		expr->getRight()->accept(this);
		patchJump(jump);
		return;
	}
	expr->getRight()->accept(this);
	current->line = expr->getToken().line;
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

void compiler::visitUnaryExpr(ASTUnaryExpr* expr) {
	expr->getRight()->accept(this);
	Token token = expr->getToken();
	current->line = token.line;
	switch (token.type) {
	case TOKEN_MINUS: emitByte(OP_NEGATE); break;
	case TOKEN_BANG: emitByte(OP_NOT); break;
	case TOKEN_TILDA: emitByte(OP_BIN_NOT); break;
	}
}

void compiler::visitArrayDeclExpr(ASTArrayDeclExpr* expr) {
	for (ASTNode* node : expr->getMembers()) {
		node->accept(this);
	}
	emitBytes(OP_CREATE_ARRAY, expr->getSize());
}

void compiler::visitCallExpr(ASTCallExpr* expr) {
	expr->getCallee()->accept(this);
	// '(' is a function call, '[' and '.' are access operators
	switch (expr->getAccessor().type) {
	case TOKEN_LEFT_PAREN: {
		int argCount = 0;
		for (ASTNode* arg : expr->getArgs()) {
			arg->accept(this);
			argCount++;
		}
		emitBytes(OP_CALL, argCount);
		break;
	}
		//these are usually things like array[index] or object.property
	case TOKEN_LEFT_BRACKET: {
		expr->getArgs()[0]->accept(this);
		emitByte(OP_GET);
		break;
	}
	case TOKEN_DOT:
		int name = identifierConstant(((ASTLiteralExpr*)expr->getArgs()[0])->getToken());
		emitBytes(OP_GET_PROPERTY, name);
		break;
	}
}

void compiler::visitGroupingExpr(ASTGroupingExpr* expr) {
	expr->getExpr()->accept(this);
}

void compiler::visitUnaryVarAlterExpr(ASTUnaryVarAlterExpr* expr) {
	//if 'field' field is null then this is a variable incrementing, if it's not null, then this is a field incrementing
	if (expr->getField() == nullptr) {
		Token varName = ((ASTLiteralExpr*)expr->getCallee())->getToken();
		namedVar(varName, false);
		int type = -1;
		int arg = resolveLocal(varName);
		if (arg != -1) {
			type = 0;
		}else if ((arg = resolveUpvalue(current, varName)) != -1) {
			type = 1;
		}else {
			arg = identifierConstant(varName);
			type = 2;
		}

		if (expr->getIsPrefix()) {
			if (expr->getOp().type == TOKEN_INCREMENT) emitByte(OP_INCREMENT_PRE);
			else emitByte(OP_DECREMENT_PRE);
		}
		else {
			if (expr->getOp().type == TOKEN_INCREMENT) emitByte(OP_INCREMENT_POST);
			else emitByte(OP_DECREMENT_POST);
		}
		emitBytes(type, arg);
	}
	else {
		//if this a dot call, we parse it differently than '[' access
		int constant = -1;
		if (expr->getOp().type == TOKEN_LEFT_BRACKET) expr->getField()->accept(this);
		else constant = identifierConstant(((ASTLiteralExpr*)expr->getField())->getToken());
		//we parse the callee last so that it's on top of the stack when we hit this
		expr->getCallee()->accept(this);

		if (expr->getIsPrefix()) {
			if (expr->getOp().type == TOKEN_INCREMENT) emitByte(OP_INCREMENT_PRE);
			else emitByte(OP_DECREMENT_PRE);
		}
		else {
			if (expr->getOp().type == TOKEN_INCREMENT) emitByte(OP_INCREMENT_POST);
			else emitByte(OP_DECREMENT_POST);
		}
		//emitting type bytes for different access operator('.' or '[')
		//3 means it's a '[' access, 4 means it's a '.' access
		if(constant == -1) emitByte(3);
		else emitBytes(4, constant);
	}
}

void compiler::visitLiteralExpr(ASTLiteralExpr* expr) {
	const Token& token = expr->getToken();
	current->line = token.line;
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
		emitConstant(OBJ_VAL(copyString((char*)_temp.c_str(), _temp.length())));
		break;
	}

	case TOKEN_IDENTIFIER: {
		namedVar(token, false);
		break;
	}
	}
}



void compiler::visitVarDecl(ASTVarDecl* decl) {
	//if this is a global, we get a string constant index, if it's a local, it returns a dummy 0
	current->line = decl->getToken().line;
	uint8_t global = parseVar(decl->getToken());
	//compile the right side of the declaration, if there is no right side, the variable is initialized as nil
	ASTNode* expr = decl->getExpr();
	if (expr == NULL) {
		emitByte(OP_NIL);
	}else{
		expr->accept(this);
	}
	//if this is a global var, we emit the code to define it in the VMs global hash table, if it's a local, do nothing
	//the slot that the compiled value is at becomes a local var
	defineVar(global);
}

void compiler::visitFuncDecl(ASTFunc* decl) {
	uint8_t name = parseVar(decl->getName());
	markInit();
	//creating a new compilerInfo sets us up with a clean slate for writing bytecode, the enclosing functions info
	//is stored in current->enclosing
	current = new compilerInfo(current, funcType::TYPE_FUNC);
	//no need for a endScope, since returning from the function discards the entire callstack
	beginScope();
	//we define the args as locals, when the function is called, the args will be sitting on the stack in order
	//we just assign those positions to each arg
	for (Token& var : decl->getArgs()) {
		uint8_t constant = parseVar(var);
		defineVar(constant);
	}
	decl->getBody()->accept(this);
	current->func->arity = decl->getArgs().size();
	//could get away with using string instead of objString?
	string str(decl->getName().lexeme);
	current->func->name = copyString((char*)str.c_str(), str.length());
	//have to do this here since endFuncDecl() deletes the compilerInfo
	std::array<upvalue, UPVAL_MAX> upvals = current->upvalues;

	objFunc* func = endFuncDecl();
	emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(func)));
	for (int i = 0; i < func->upvalueCount; i++) {
		emitByte(upvals[i].isLocal ? 1 : 0);
		emitByte(upvals[i].index);
	}
	defineVar(name);
}

void compiler::visitClassDecl(ASTClass* decl) {
	Token className = decl->getName();
	current->line = className.line;
	int constant = identifierConstant(className);
	declareVar(className);

	emitBytes(OP_CLASS, constant);
	defineVar(constant);
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
	int loopStart = getChunk()->code.size();
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
	int loopStart = getChunk()->code.size();
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
	//patch breaks AFTER we close the for scope because breaks that are in current scope aren't patched
	patchBreak();
}

void compiler::visitBreakStmt(ASTBreakStmt* stmt) {
	//the amount of variables to pop and the amount of code to jump is determined in handleBreak()
	//which is called at the end of loops
	current->line = stmt->getToken().line;
	emitByte(OP_BREAK);
	int breakJump = getChunk()->code.size();
	emitBytes(0xff, 0xff);
	emitBytes(0xff, 0xff);
	current->breakStmts.emplace_back(current->scopeDepth, breakJump, current->localCount);
}

void compiler::visitSwitchStmt(ASTSwitchStmt* stmt) {
	beginScope();
	//compile the expression in ()
	stmt->getExpr()->accept(this);
	//based on the switch stmt type(all num, all string or mixed) we create a new switch table struct and get it's pos
	//passing in the size to call .reserve() on map/vector
	switchType type = stmt->getType();
	int pos = getChunk()->addSwitch(switchTable(type, stmt->getCases().size()));
	switchTable& table = getChunk()->switchTables[pos];
	emitBytes(OP_SWITCH, pos);

	long start = getChunk()->code.size();
	//TODO:defaults
	for (ASTNode* _case : stmt->getCases()) {
		ASTCase* curCase = (ASTCase*)_case;
		//based on the type of switch stmt, we either convert all token lexemes to numbers,
		//or add the strings to a hash table
		if (!curCase->getDef()) {
			switch (type) {
			case switchType::NUM: {
				ASTLiteralExpr* expr = (ASTLiteralExpr*)curCase->getExpr();
				//TODO: round this?
				int key = std::stoi(string(expr->getToken().lexeme));
				long _ip = getChunk()->code.size() - start;

				table.addToArr(key, _ip);
				break;
			}
								//for both strings and mixed switches we pass them as strings
			case switchType::STRING:
			case switchType::MIXED: {
				ASTLiteralExpr* expr = (ASTLiteralExpr*)curCase->getExpr();
				long _ip = getChunk()->code.size() - start;
				//converts string_view to string and get's rid of ""
				string _temp(expr->getToken().lexeme);
				if (expr->getToken().type == TOKEN_STRING) {
					_temp.erase(0, 1);
					_temp.erase(_temp.size() - 1, 1);
				}
				table.addToTable(_temp, _ip);
				break;
			}
			}
		}
		else {
			table.defaultJump = getChunk()->code.size() - start;
		}
		curCase->accept(this);
	}
	//implicit default if the user hasn't defined one, jumps to the end of switch stmt
	if (table.defaultJump == -1) table.defaultJump = getChunk()->code.size() - start;
	//we use a scope and patch breaks AFTER ending the scope because breaks that are in the current scope aren't patched
	endScope();
	patchBreak();
}

void compiler::visitCase(ASTCase* stmt) {
	//compile every statement in the case
	//user has to worry about fallthrough
	for (ASTNode* stmt : stmt->getStmts()) {
		stmt->accept(this);
	}
}

void compiler::visitReturnStmt(ASTReturn* stmt) {
	if (current->type == funcType::TYPE_SCRIPT) {
		error("Can't return from top-level code.");
	}
	if (stmt->getExpr() == NULL) {
		emitReturn();
		return;
	}
	stmt->getExpr()->accept(this);
	emitByte(OP_RETURN);
	current->hasReturn = true;
}

#pragma region helpers

#pragma region Emitting bytes

void compiler::emitByte(uint8_t byte) {
	getChunk()->writeData(byte, current->line);//line is incremented whenever we find a statement/expression that contains tokens
}

void compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
	emitByte(byte1);
	emitByte(byte2);
}

void compiler::emit16Bit(unsigned short number) {
	emitBytes((number >> 8) & 0xff, number & 0xff);
}

uint8_t compiler::makeConstant(Value value) {
	int constant = getChunk()->addConstant(value);
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
	emitByte(OP_NIL);
	emitByte(OP_RETURN);
}

int compiler::emitJump(OpCode jumpType) {
	emitByte((uint8_t)jumpType);
	emitBytes(0xff, 0xff);
	return getChunk()->code.size() - 2;
}

void compiler::patchJump(int offset) {
	// -2 to adjust for the bytecode for the jump offset itself.
	int jump = getChunk()->code.size() - offset - 2;
	//fix for future: insert 2 more bytes into the array, but make sure to do the same in lines array
	if (jump > UINT16_MAX) {
		error("Too much code to jump over.");
	}
	getChunk()->code[offset] = (jump >> 8) & 0xff;
	getChunk()->code[offset + 1] = jump & 0xff;
}

void compiler::emitLoop(int start) {
	emitByte(OP_LOOP);

	int offset = getChunk()->code.size() - start + 2;
	if (offset > UINT16_MAX) error("Loop body too large.");

	emit16Bit(offset);
}

#pragma endregion

#pragma region Variables

bool identifiersEqual(string* a, string* b) {
	return (a->size() == b->size() && a->compare(*b) == 0);
}

uint8_t compiler::identifierConstant(Token name) {
	string temp(name.lexeme);
	return makeConstant(OBJ_VAL(copyString((char*)temp.c_str(), temp.length())));
}

void compiler::defineVar(uint8_t name) {
	//if this is a local var, bail out
	if (current->scopeDepth > 0) { 
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
	}else if ((arg = resolveUpvalue(current, token)) != -1) {
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
	}else {
		arg = identifierConstant(token);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}
	emitBytes(canAssign ? setOp : getOp, arg);
}

uint8_t compiler::parseVar(Token name) {
	declareVar(name);
	if (current->scopeDepth > 0) return 0;
	return identifierConstant(name);
}

void compiler::declareVar(Token& name) {
	if (current->scopeDepth == 0) return;
	for (int i = current->localCount - 1; i >= 0; i--) {
		local* _local = &current->locals[i];
		if (_local->depth != -1 && _local->depth < current->scopeDepth) {
			break;
		}
		string str = string(name.lexeme);
		if (identifiersEqual(&str, &_local->name)) {
			error("Already a variable with this name in this scope.");
		}
	}
	addLocal(name);
}

void compiler::addLocal(Token name) {
	if (current->localCount == LOCAL_MAX) {
		error("Too many local variables in function.");
		return;
	}
	local* _local = &current->locals[current->localCount++];
	_local->name = name.lexeme;
	_local->depth = -1;
}

void compiler::endScope() {
	//Pop every variable that was declared in this scope
	current->scopeDepth--;//first lower the scope, the check for every var that is deeper than the current scope
	int toPop = 0;
	while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
		if(!current->hasCapturedLocals) toPop++;
		else {
			if (current->locals[current->localCount - 1].isCaptured) {
				emitByte(OP_CLOSE_UPVALUE);
			}
			else {
				emitByte(OP_POP);
			}
		}
		current->localCount--;
	}
	if(toPop > 0) emitBytes(OP_POPN, toPop);
}

int compiler::resolveLocal(compilerInfo* func, Token& name) {
	//checks to see if there is a local variable with a provided name, if there is return the index of the stack slot of the var
	for (int i = func->localCount - 1; i >= 0; i--) {
		local* _local = &func->locals[i];
		string str = string(name.lexeme);
		if (identifiersEqual(&str, &_local->name)) {
			if (_local->depth == -1) {
				error("Can't read local variable in its own initializer.");
			}
			return i;
		}
	}

	return -1;
}

int compiler::resolveLocal(Token& name) {
	return resolveLocal(current, name);
}

int compiler::resolveUpvalue(compilerInfo* func, Token& name) {
	if (func->enclosing == NULL) return -1;

	int local = resolveLocal(func->enclosing, name);
	if (local != -1) {
		func->enclosing->locals[local].isCaptured = true;
		func->hasCapturedLocals = true;
		return addUpvalue((uint8_t)local, true);
	}

	int upvalue = resolveUpvalue(func->enclosing, name);
	if (upvalue != -1) {
		return addUpvalue((uint8_t)upvalue, false);
	}

	return -1;
}

int compiler::addUpvalue(uint8_t index, bool isLocal) {
	int upvalueCount = current->func->upvalueCount;
	for (int i = 0; i < upvalueCount; i++) {
		upvalue* upval = &current->upvalues[i];
		if (upval->index == index && upval->isLocal == isLocal) {
			return i;
		}
	}
	if (upvalueCount == UPVAL_MAX) {
		error("Too many closure variables in function.");
		return 0;
	}
	current->upvalues[upvalueCount].isLocal = isLocal;
	current->upvalues[upvalueCount].index = index;
	return current->func->upvalueCount++;
}

void compiler::markInit() {
	if (current->scopeDepth == 0) return;
	//marks variable as ready to use, any use of it before this call is a error
	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

void compiler::patchBreak() {
	int curCode = getChunk()->code.size();
	//most recent breaks are going to be on top
	for (int i = current->breakStmts.size() - 1; i >= 0; i--) {
		_break curBreak = current->breakStmts[i];
		//if the break stmt we're looking at has a depth that's equal or higher than current depth, we bail out
		if (curBreak.depth > current->scopeDepth) {
			int jumpLenght = curCode - curBreak.offset - 4;
			int toPop = curBreak.varNum - current->localCount;
			//variables declared by the time we hit the break whose depth is lower or equal to this break stmt
			getChunk()->code[curBreak.offset] = (toPop >> 8) & 0xff;
			getChunk()->code[curBreak.offset + 1] = toPop & 0xff;
			//amount to jump
			getChunk()->code[curBreak.offset + 2] = (jumpLenght >> 8) & 0xff;
			getChunk()->code[curBreak.offset + 3] = jumpLenght & 0xff;
			//delete break from array
			current->breakStmts.pop_back();
		}else break;
	}
}

#pragma endregion

chunk* compiler::getChunk() {
	return &current->func->body;
}

void error(string message) {
	std::cout << "Compile error: " << message << "\n";
	throw 20;
}

objFunc* compiler::endFuncDecl() {
	if(!current->hasReturn) emitReturn();
	#ifdef DEBUG_PRINT_CODE
		current->func->body.disassemble(current->func->name == nullptr ? "script" : current->func->name->str);
	#endif
	//get the current function we've just compiled, delete it's compiler info, and replace it with the enclosing functions compiler info
	objFunc* func = current->func;
	compilerInfo* temp = current->enclosing;
	delete current;
	current = temp;
	return func;
}

#pragma endregion
