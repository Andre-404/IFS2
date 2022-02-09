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
	if (type != funcType::TYPE_FUNC) {
		_local->name = "this";
	}
	else {
		_local->name = "";
	}
	func = new objFunc();
	code = &func->body;
}


compiler::compiler(parser* _parser, funcType _type) {
	Parser = _parser;
	compiled = true;
	global::gc.compiling = this;
	current = new compilerInfo(nullptr, funcType::TYPE_SCRIPT);
	currentClass = nullptr;
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

bool identifiersEqual(std::string_view a, std::string_view b);


void compiler::visitAssignmentExpr(ASTAssignmentExpr* expr) {
	expr->getVal()->accept(this);//compile the right side of the expression
	updateLine(expr->getToken());
	namedVar(expr->getToken(), true);
}

void compiler::visitSetExpr(ASTSetExpr* expr) {
	//different behaviour for '[' and '.'
	updateLine(expr->getAccessor());
	switch (expr->getAccessor().type) {
	case TOKEN_LEFT_BRACKET: {
		expr->getCallee()->accept(this);
		expr->getField()->accept(this);
		expr->getValue()->accept(this);
		emitByte(OP_SET);
		break;
	}
	//the "." is always followed by a field name as a string, so we can make it a constant and emit the position of the constant instead
	case TOKEN_DOT: {
		expr->getCallee()->accept(this);
		expr->getValue()->accept(this);
		int name = identifierConstant(((ASTLiteralExpr*)expr->getField())->getToken());
		emitBytes(OP_SET_PROPERTY, name);
		break;
	}
	}
}

void compiler::visitConditionalExpr(ASTConditionalExpr* expr) {
	//compile condition and emit a jump over then branch if the condition is false
	expr->getCondition()->accept(this);
	int thenJump = emitJump(OP_JUMP_IF_FALSE_POP);
	expr->getThenBranch()->accept(this);
	//prevents fallthrough to else branch
	int elseJump = emitJump(OP_JUMP);
	patchJump(thenJump);
	//no need to check if a else branch exists since it's required by the conditional statement
	expr->getElseBranch()->accept(this);
	patchJump(elseJump);
}

void compiler::visitBinaryExpr(ASTBinaryExpr* expr) {
	updateLine(expr->getToken());
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
	case TOKEN_BITWISE_AND: emitByte(OP_BITWISE_AND); break;
	case TOKEN_BITWISE_OR: emitByte(OP_BITWISE_OR); break;
	case TOKEN_BITWISE_XOR: emitByte(OP_BITWISE_XOR); break;
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
	updateLine(token);
	switch (token.type) {
	case TOKEN_MINUS: emitByte(OP_NEGATE); break;
	case TOKEN_BANG: emitByte(OP_NOT); break;
	case TOKEN_TILDA: emitByte(OP_BIN_NOT); break;
	}
}

void compiler::visitArrayDeclExpr(ASTArrayDeclExpr* expr) {
	//we need all of the array member values to be on the stack prior to executing "OP_CREATE_ARRAY"
	for (ASTNode* node : expr->getMembers()) {
		node->accept(this);
	}
	emitBytes(OP_CREATE_ARRAY, expr->getSize());
}

void compiler::visitCallExpr(ASTCallExpr* expr) {
	//invoking is field access + call, when the compiler recognizes this pattern it optimizes
	if(invoke(expr)) return;
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
	//these are things like array[index] or object.property(or object["propertyAsString"])
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
	//type => 0: local, 1: upvalue, 2: global, 3: '[' access, 4: '.' access
	if (expr->getField() == nullptr) {
		Token varName = ((ASTLiteralExpr*)expr->getCallee())->getToken();
		updateLine(varName);
		namedVar(varName, false);
		//we can't use namedVar here because prefix and postfix incrementing/decrementing push the value of the variable at different times
		//(before or after the incrementation)
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
		}else {
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

void compiler::visitStructLiteralExpr(ASTStructLiteral* expr) {
	vector<structEntry>& entries = expr->getEntries();

	vector<int> constants;

	//we first compile each expression, and then get it's field name
	for (structEntry entry : entries) {
		entry.expr->accept(this);
		updateLine(entry.name);
		constants.push_back(identifierConstant(entry.name));
	}
	//since the amount of fields is variable, we emit the number of fields follwed by constants for each field
	//it's extremely important that we emit the constants in reverse order to how we pushed them, this is because when the VM gets to this point
	//all of the values we need will be on the stack, and getting them from the back(by popping) will give us the values in reverse order
	//compared to how we compiled them
	emitBytes(OP_CREATE_STRUCT, constants.size());
	for (int i = constants.size() - 1; i >= 0; i--) emitByte(constants[i]);
}

void compiler::visitSuperExpr(ASTSuperExpr* expr) {
	updateLine(expr->getName());
	int name = identifierConstant(expr->getName());
	if (currentClass == nullptr) {
		error("Can't use 'super' outside of a class.");
	}
	else if (!currentClass->hasSuperclass) {
		error("Can't use 'super' in a class with no superclass.");
	}
	//we use syntethic tokens since we know that 'super' and 'this' are defined if we're currently compiling a class method
	namedVar(syntheticToken("this"), false);
	namedVar(syntheticToken("super"), false);
	emitBytes(OP_GET_SUPER, name);
}

void compiler::visitLiteralExpr(ASTLiteralExpr* expr) {
	const Token& token = expr->getToken();
	updateLine(token);
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

	case TOKEN_THIS: {
		if (currentClass == nullptr) {
			error("Can't use 'this' outside of a class.");
			break;
		}
		//'this' get implicitly defined by the compiler
		namedVar(token, false);
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
	updateLine(decl->getToken());
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
	updateLine(decl->getName());
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
		updateLine(var);
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
	//if this function does capture any upvalues, we emit the code for getting them, 
	//when we execute "OP_CLOSURE" we will check to see how many upvalues the function captures by going directly to the func->upvalueCount
	for (int i = 0; i < func->upvalueCount; i++) {
		emitByte(upvals[i].isLocal ? 1 : 0);
		emitByte(upvals[i].index);
	}
	defineVar(name);
}

void compiler::visitClassDecl(ASTClass* decl) {
	Token className = decl->getName();
	updateLine(className);
	int constant = identifierConstant(className);
	declareVar(className);

	emitBytes(OP_CLASS, constant);
	//define the class here, so that we can use it inside it's own methods
	defineVar(constant);

	classCompilerInfo temp(currentClass, false);
	currentClass = &temp;

	if (decl->inherits()) {
		//if the class does inherit from some other class, we load the parent class and declare 'super' as a local variable
		//which holds said class
		namedVar(decl->getInherited(), false);
		if (identifiersEqual(className.lexeme, decl->getInherited().lexeme)) {
			error("A class can't inherit from itself.");
		}
		beginScope();
		addLocal(syntheticToken("super"));
		defineVar(0);

		namedVar(className, false);
		emitByte(OP_INHERIT);
		currentClass->hasSuperclass = true;
	}
	//we need to load the class onto the top of the stack so that 'this' keyword can work correctly inside of methods
	//the class that 'this' refers to is captured as a upvalue inside of methods
	if(!decl->inherits()) namedVar(className, false);
	for (ASTNode* _method : decl->getMethods()) {
		method((ASTFunc*)_method, className);
	}
	//popping the current class
	emitByte(OP_POP);

	if (currentClass->hasSuperclass) {
		endScope();
	}
	currentClass = currentClass->enclosing;
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

void compiler::visitForeachStmt(ASTForeachStmt* stmt) {
	beginScope();
	//these synthetic tokens are used for variables that can never(and should never) be accessed by the user
	Token iterator = syntheticToken("0");
	//get the iterator
	stmt->getCollection()->accept(this);
	emitByte(OP_ITERATOR_START);
	addLocal(iterator);
	defineVar(0);
	//Get the var ready
	emitByte(OP_NIL);
	addLocal(stmt->getVarName());
	defineVar(0);

	int loopStart = getChunk()->code.size();

	//advancing
	namedVar(iterator, false);
	emitByte(OP_ITERATOR_NEXT);
	int jump = emitJump(OP_JUMP_IF_FALSE_POP);

	//get new variable
	namedVar(iterator, false);
	emitByte(OP_ITERATOR_GET);
	namedVar(stmt->getVarName(), true);
	emitByte(OP_POP);

	stmt->getBody()->accept(this);

	//end of loop
	emitLoop(loopStart);
	patchJump(jump);
	endScope();
	patchBreak();
}

void compiler::visitBreakStmt(ASTBreakStmt* stmt) {
	//the amount of variables to pop and the amount of code to jump is determined in handleBreak()
	//which is called at the end of loops
	updateLine(stmt->getToken());
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
	for (ASTNode* _case : stmt->getCases()) {
		ASTCase* curCase = (ASTCase*)_case;
		//based on the type of switch stmt, we either convert all token lexemes to numbers,
		//or add the strings to a hash table
		if (!curCase->getDef()) {
			switch (type) {
			case switchType::NUM: {
				ASTLiteralExpr* expr = (ASTLiteralExpr*)curCase->getExpr();
				updateLine(expr->getToken());
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
				updateLine(expr->getToken());
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
	if (current->type == funcType::TYPE_CONSTRUCTOR) {
		error("Can't return a value from an constructor.");
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
	if (current->type == funcType::TYPE_CONSTRUCTOR) emitBytes(OP_GET_LOCAL, 0);
	else emitByte(OP_NIL);
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
	return (a->compare(*b) == 0);
}

bool identifiersEqual(std::string_view a, std::string_view b) {
	return (a.compare(b) == 0);
}

uint8_t compiler::identifierConstant(Token name) {
	string temp(name.lexeme);
	return makeConstant(OBJ_VAL(copyString((char*)temp.c_str(), temp.length())));
}

void compiler::defineVar(uint8_t name) {
	//if this is a local var, mark it as ready and then bail out
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
	if(toPop > 0 && !current->hasCapturedLocals) emitBytes(OP_POPN, toPop);
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
		func->enclosing->hasCapturedLocals = true;
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

Token compiler::syntheticToken(const char* str) {
	Token token;
	token.lexeme = str;
	token.line = current->line;
	return token;
}

#pragma endregion

#pragma region Classes and methods
void compiler::method(ASTFunc* _method, Token className) {
	int name = identifierConstant(_method->getName());
	//creating a new compilerInfo sets us up with a clean slate for writing bytecode, the enclosing functions info
	//is stored in current->enclosing
	funcType type = funcType::TYPE_METHOD;
	//constructors are treated separatly, but are still methods
	if (_method->getName().lexeme.compare(className.lexeme) == 0) type = funcType::TYPE_CONSTRUCTOR;
	current = new compilerInfo(current, type);
	//no need for a endScope, since returning from the function discards the entire callstack
	beginScope();
	//we define the args as locals, when the function is called, the args will be sitting on the stack in order
	//we just assign those positions to each arg
	for (Token& var : _method->getArgs()) {
		uint8_t constant = parseVar(var);
		defineVar(constant);
	}
	_method->getBody()->accept(this);
	current->func->arity = _method->getArgs().size();
	//could get away with using string instead of objString?
	string str(_method->getName().lexeme);
	current->func->name = copyString((char*)str.c_str(), str.length());
	//have to do this here since endFuncDecl() deletes the compilerInfo
	std::array<upvalue, UPVAL_MAX> upvals = current->upvalues;

	objFunc* func = endFuncDecl();
	emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(func)));
	for (int i = 0; i < func->upvalueCount; i++) {
		emitByte(upvals[i].isLocal ? 1 : 0);
		emitByte(upvals[i].index);
	}
	emitBytes(OP_METHOD, name);
}

bool compiler::invoke(ASTCallExpr* expr) {
	if (expr->getCallee()->type == ASTType::CALL) {
		//currently we only optimizes field invoking(struct.field())
		ASTCallExpr* _call = (ASTCallExpr*)expr->getCallee();
		if (_call->getAccessor().type != TOKEN_DOT) return false;

		_call->getCallee()->accept(this);
		int field = identifierConstant(((ASTLiteralExpr*)_call->getArgs()[0])->getToken());
		int argCount = 0;
		for (ASTNode* arg : expr->getArgs()) {
			arg->accept(this);
			argCount++;
		}
		emitBytes(OP_INVOKE, field);
		emitByte(argCount);
		return true;
	}else if (expr->getCallee()->type == ASTType::SUPER) {
		ASTSuperExpr* _superCall = (ASTSuperExpr*)expr->getCallee();
		int name = identifierConstant(_superCall->getName());

		if (currentClass == nullptr) {
			error("Can't use 'super' outside of a class.");
		}
		else if (!currentClass->hasSuperclass) {
			error("Can't use 'super' in a class with no superclass.");
		}

		namedVar(syntheticToken("this"), false);
		int argCount = 0;
		for (ASTNode* arg : expr->getArgs()) {
			arg->accept(this);
			argCount++;
		}
		//super gets popped, leaving only the receiver and args on the stack
		namedVar(syntheticToken("super"), false);
		emitBytes(OP_SUPER_INVOKE, name);
		emitByte(argCount);
	}
	return false;
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

//a little helper for updating the lines emitted by the compiler(used for displaying runtime errors)
void compiler::updateLine(Token token) {
	current->line = token.line;
}

#pragma endregion
