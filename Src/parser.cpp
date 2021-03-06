#include "parser.h"
#include "AST.h"
#include "debug.h"
#include "preprocessor.h"


translationUnit::translationUnit(vector<translationUnit*>& sortedUnits, preprocessUnit* pUnit) {
	name = pUnit->name;
	src = pUnit->srcFile;
	for (preprocessUnit* unit : pUnit->deps) {
		string name = unit->name;
		//guaranteed to work
		for (translationUnit* dep : sortedUnits) {
			if (dep->name == name) {
				deps.push_back(dep);
				break;
			}
		}
	}
}

#pragma region Parselet
//used for parsing assignment tokens(eg. =, +=, *=...)
ASTNode* parser::parseAssign(ASTNode* left, Token op) {
	ASTNode* right = expression();
	switch (op.type) {
	case TOKEN_EQUAL: {
		break;
	}
	case TOKEN_PLUS_EQUAL: {
		right = new ASTBinaryExpr(left, Token("+", op.line, TOKEN_PLUS), right);
		break;
	}
	case TOKEN_MINUS_EQUAL: {
		right = new ASTBinaryExpr(left, Token("-", op.line, TOKEN_MINUS), right);
		break;
	}
	case TOKEN_SLASH_EQUAL: {
		right = new ASTBinaryExpr(left, Token("/", op.line, TOKEN_SLASH), right);
		break;
	}
	case TOKEN_STAR_EQUAL: {
		right = new ASTBinaryExpr(left, Token("*", op.line, TOKEN_STAR), right);
		break;
	}
	case TOKEN_BITWISE_XOR_EQUAL: {
		right = new ASTBinaryExpr(left, Token("^", op.line, TOKEN_BITWISE_XOR), right);
		break;
	}
	case TOKEN_BITWISE_AND_EQUAL: {
		right = new ASTBinaryExpr(left, Token("&", op.line, TOKEN_BITWISE_AND), right);
		break;
	}
	case TOKEN_BITWISE_OR_EQUAL: {
		right = new ASTBinaryExpr(left, Token("|", op.line, TOKEN_BITWISE_OR), right);
		break;
	}
	case TOKEN_PERCENTAGE_EQUAL: {
		right = new ASTBinaryExpr(left, Token("%", op.line, TOKEN_PERCENTAGE), right);
		break;
	}
	}
	return right;
}

class unaryExpr : public prefixParselet {
	ASTNode* parse(Token token) {
		ASTNode* expr = cur->expression(prec);
		return new ASTUnaryExpr(token, expr);
	}
};

class literalExpr : public prefixParselet {
	ASTNode* parse(Token token) {
		switch (token.type) {
		case TOKEN_SUPER: {
			cur->consume(TOKEN_DOT, "Expected '.' after super.");
			Token ident = cur->consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
			return new ASTSuperExpr(ident);
		}
		case TOKEN_LEFT_PAREN:{
			//grouping can contain a expr of any precedence
			ASTNode* expr = cur->expression();
			cur->consume(TOKEN_RIGHT_PAREN, "Expected ')' at the end of grouping expression.");
			return new ASTGroupingExpr(expr);
		}
		//Array literal
		case TOKEN_LEFT_BRACKET: {
			vector<ASTNode*> members;
			if (!(cur->peek().type == TOKEN_RIGHT_BRACKET)) {
				do {
					members.push_back(cur->expression());
				} while (cur->match({ TOKEN_COMMA }));
			}
			cur->consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array initialization.");
			return new ASTArrayDeclExpr(members);
		}
		//Struct literal
		case TOKEN_LEFT_BRACE: {
			vector<structEntry> entries;
			if (!(cur->peek().type == TOKEN_RIGHT_BRACE)) {
				//a struct literal looks like this: {var1 : expr1, var2 : expr2}
				do {
					Token identifier = cur->consume(TOKEN_IDENTIFIER, "Expected a identifier.");
					cur->consume(TOKEN_COLON, "Expected a ':' after identifier");
					ASTNode* expr = cur->expression();
					entries.emplace_back(identifier, expr);
				} while (cur->match({ TOKEN_COMMA }));
			}
			cur->consume(TOKEN_RIGHT_BRACE, "Expect '}' after struct literal.");
			return new ASTStructLiteral(entries);
		}
		case TOKEN_YIELD: {
			ASTNode* expr = nullptr;
			if (cur->peek().type == TOKEN_COLON) {
				expr = cur->expression();
			}
			return new ASTYieldExpr(expr);
		}
		case TOKEN_FIBER: {
			//the depths are used for throwing errors for switch and loops stmts, and since a function can be declared inside a loop we need to account
			//for that
			int tempLoopDepth = cur->loopDepth;
			cur->loopDepth = 0;
			int tempSwitchDepth = cur->switchDepth;
			cur->switchDepth = 0;

			cur->consume(TOKEN_LEFT_PAREN, "Expect '(' for fiber starting parameters.");
			vector<Token> args;
			int arity = 0;
			if (!cur->check(TOKEN_RIGHT_PAREN)) {
				do {
					Token arg = cur->consume(TOKEN_IDENTIFIER, "Expect argument name");
					args.push_back(arg);
					arity++;
					if (arity > 255) {
						throw cur->error(arg, "Fibers can't have more than 255 starting params.");
					}
				} while (cur->match({ TOKEN_COMMA }));
			}
			cur->consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
			cur->consume(TOKEN_LEFT_BRACE, "Expect '{' after arguments.");
			ASTNode* body = cur->blockStmt();

			cur->loopDepth = tempLoopDepth;
			cur->switchDepth = tempSwitchDepth;
			return new ASTFiberLiteral(args, arity, body);
		}
		default:
			return new ASTLiteralExpr(token);
		}
	}
};

class unaryVarAlterPrefix : public prefixParselet {
	ASTNode* parse(Token op) {
		ASTNode* var = cur->expression(prec);
		ASTUnaryVarAlterExpr* expr = nullptr;
		ASTLiteralExpr* one = new ASTLiteralExpr(Token("1", op.line, TOKEN_NUMBER));
		Token _op = op.type == TOKEN_INCREMENT ? Token("+", op.line, TOKEN_PLUS) : Token("-", op.line, TOKEN_MINUS);
		bool isPositive = op.type == TOKEN_INCREMENT;
		//differentiate between variable incrementation, and (array or struct) field incrementation
		if (var->type == ASTType::LITERAL && ((ASTLiteralExpr*)var)->getToken().type == TOKEN_IDENTIFIER) {
			//++var gets translated to var = var + 1
			Token varName = ((ASTLiteralExpr*)var)->getToken();
			ASTBinaryExpr* binary = new ASTBinaryExpr(var, _op, one);
			expr = new ASTUnaryVarAlterExpr(new ASTAssignmentExpr(varName, binary), true, isPositive);
		}
		else if (var->type == ASTType::CALL) {
			ASTCallExpr* call = (ASTCallExpr*)var;
			//if the accessor token type is a left paren, then this is a function call, and not a get expression, so we throw a error
			if (call->getAccessor().type == TOKEN_LEFT_PAREN) throw cur->error(op, "Can't increment a r-value.");

			ASTBinaryExpr* binary = new ASTBinaryExpr(call, _op, one);
			expr = new ASTUnaryVarAlterExpr(new ASTSetExpr(call->getCallee(), call->getArgs()[0], op, binary), true, isPositive);
		}
		else {
			throw cur->error(op, "Can't increment a r-value");
		}
		return expr;
	}
};

class assignmentExpr : public infixParselet {
	ASTNode* parse(ASTNode* left, Token token, int surroundingPrec) {
		//makes it right associative
		ASTNode* right = cur->parseAssign(left, token);
		if (left->type != ASTType::LITERAL && ((ASTLiteralExpr*)left)->getToken().type != TOKEN_IDENTIFIER) {
			throw cur->error(token, "Left side is not assignable");
		}
		ASTAssignmentExpr* expr = new ASTAssignmentExpr(((ASTLiteralExpr*)left)->getToken(), right);
		return expr;
	}
};

class conditionalExpr : public infixParselet {
	ASTNode* parse(ASTNode* left, Token token, int surroundingPrec) {
		ASTNode* thenBranch = cur->expression(prec - 1);
		cur->consume(TOKEN_COLON, "Expected ':' after then branch.");
		ASTNode* elseBranch = cur->expression(prec - 1);
		return new ASTConditionalExpr(left, thenBranch, elseBranch);
	}
};

class binaryExpr : public infixParselet {
	ASTNode* parse(ASTNode* left, Token token, int surroundingPrec) {
		ASTNode* right = cur->expression(prec);
		return new ASTBinaryExpr(left, token, right);
	}
};

class unaryVarAlterPostfix : public infixParselet {
	ASTNode* parse(ASTNode* var, Token op, int surroundingPrec) {
		ASTUnaryVarAlterExpr* expr = nullptr;
		ASTLiteralExpr* one = new ASTLiteralExpr(Token("1", op.line, TOKEN_NUMBER));
		Token _op = op.type == TOKEN_INCREMENT ? Token("+", op.line, TOKEN_PLUS) : Token("-", op.line, TOKEN_MINUS);
		bool isPositive = op.type == TOKEN_INCREMENT;
		//differentiate between variable incrementation, and (array or struct) field incrementation
		if (var->type == ASTType::LITERAL && ((ASTLiteralExpr*)var)->getToken().type == TOKEN_IDENTIFIER) {
			//var++ gets translated to var = var + 1
			Token varName = ((ASTLiteralExpr*)var)->getToken();
			ASTBinaryExpr* binary = new ASTBinaryExpr(var, _op, one);
			expr = new ASTUnaryVarAlterExpr(new ASTAssignmentExpr(varName, binary), false, isPositive);
		}
		else if (var->type == ASTType::CALL) {
			ASTCallExpr* call = (ASTCallExpr*)var;
			//if the accessor token type is a left paren, then this is a function call, and not a get expression, so we throw a error
			if (call->getAccessor().type == TOKEN_LEFT_PAREN) throw cur->error(op, "Can't increment a r-value.");

			ASTBinaryExpr* binary = new ASTBinaryExpr(call, _op, one);
			expr = new ASTUnaryVarAlterExpr(new ASTSetExpr(call->getCallee(), call->getArgs()[0], op, binary), false, isPositive);
		}
		else {
			throw cur->error(op, "Can't increment a r-value");
		}
		return expr;
	}
};

class callExpr : public infixParselet {
public:
	ASTNode* parse(ASTNode* left, Token token, int surroundingPrec) {
		vector<ASTNode*> args;
		if (!cur->check(TOKEN_RIGHT_PAREN)) {
			do {
				args.push_back(cur->expression());
			} while (cur->match({ TOKEN_COMMA }));
		}
		cur->consume(TOKEN_RIGHT_PAREN, "Expect ')' after call expression.");
		return new ASTCallExpr(left, token, args);
	}
};

class fieldAccessExpr : public infixParselet {
public:
	ASTNode* parse(ASTNode* left, Token token, int surroundingPrec) {
		vector<ASTNode*> args;
		if (token.type == TOKEN_LEFT_BRACKET) {//array/struct with string access
			args.push_back(cur->expression());
			cur->consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array/map access.");
		}
		else if (token.type == TOKEN_DOT) {//struct/object access
			Token field = cur->consume(TOKEN_IDENTIFIER, "Expected a field identifier.");
			args.push_back(new ASTLiteralExpr(field));
		}
		//if we have something like arr[0] = 1 or struct.field = 1 we can't parse it with the assignment expr
		//this handles that case and produces a special set expr
		//we also check the precedence level of the surrounding expression, so "a + b.c = 3" doesn't get parsed
		//the huge match() covers every possible type of assignment
		if (surroundingPrec <= (int)precedence::ASSIGNMENT && cur->match({ TOKEN_EQUAL, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_SLASH_EQUAL,
				TOKEN_STAR_EQUAL, TOKEN_BITWISE_XOR_EQUAL, TOKEN_BITWISE_AND_EQUAL, TOKEN_BITWISE_OR_EQUAL, TOKEN_PERCENTAGE_EQUAL })) {
			ASTNode* val = cur->parseAssign(new ASTCallExpr(left, token, args), cur->previous());
			//args[0] because there is only every 1 argument inside [ ] when accessing/setting a field
			return new ASTSetExpr(left, args[0], token, val);
		}
		return new ASTCallExpr(left, token, args);
	}
};

class fiberRunExpr : public prefixParselet {
	ASTNode* parse(Token token) {
		vector<ASTNode*> args;
		cur->consume(TOKEN_LEFT_PAREN, "Expect '(' after run.");
		if (!cur->check(TOKEN_RIGHT_PAREN)) {
			do {
				args.push_back(cur->expression());
			} while (cur->match({ TOKEN_COMMA }));
		}
		cur->consume(TOKEN_RIGHT_PAREN, "Expect ')' after call expression.");
		if (args.size() == 0) {
			throw cur->error(token, "Expect atleast one argument for run(a fiber).");
		}
		return new ASTFiberRunExpr(args);
	}
};

#pragma endregion

parser::parser() {
	current = 0;
	hadError = false;
	loopDepth = 0;
	switchDepth = 0;
	curUnit = nullptr;

	#pragma region Parselets
		//Prefix
		addPrefix(TOKEN_THIS, new literalExpr, precedence::NONE);

		addPrefix(TOKEN_BANG,  new unaryExpr, precedence::NOT);
		addPrefix(TOKEN_MINUS, new unaryExpr, precedence::NOT);
		addPrefix(TOKEN_TILDA, new unaryExpr, precedence::NOT);

		addPrefix(TOKEN_INCREMENT, new unaryVarAlterPrefix, precedence::ALTER);
		addPrefix(TOKEN_DECREMENT, new unaryVarAlterPrefix, precedence::ALTER);


		addPrefix(TOKEN_IDENTIFIER,		new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_STRING,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_NUMBER,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_TRUE,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_FALSE,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_NIL,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_LEFT_PAREN,		new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_LEFT_BRACKET,	new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_LEFT_BRACE,		new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_SUPER,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_YIELD,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_FIBER,			new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_RUN, 			new fiberRunExpr, precedence::PRIMARY);

		//Infix
		addInfix(TOKEN_EQUAL,				new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_PLUS_EQUAL,			new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_MINUS_EQUAL,			new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_SLASH_EQUAL,			new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_STAR_EQUAL,			new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_PERCENTAGE_EQUAL,	new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_BITWISE_XOR_EQUAL,	new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_BITWISE_OR_EQUAL,	new assignmentExpr, precedence::ASSIGNMENT);
		addInfix(TOKEN_BITWISE_AND_EQUAL,	new assignmentExpr, precedence::ASSIGNMENT);

		addInfix(TOKEN_QUESTIONMARK, new conditionalExpr, precedence::CONDITIONAL);

		addInfix(TOKEN_OR,	new binaryExpr, precedence::OR);
		addInfix(TOKEN_AND, new binaryExpr, precedence::AND);

		addInfix(TOKEN_BITWISE_OR,  new binaryExpr, precedence::BIN_OR);
		addInfix(TOKEN_BITWISE_XOR, new binaryExpr, precedence::BIN_XOR);
		addInfix(TOKEN_BITWISE_AND, new binaryExpr, precedence::BIN_AND);

		addInfix(TOKEN_EQUAL_EQUAL, new binaryExpr, precedence::EQUALITY);
		addInfix(TOKEN_BANG_EQUAL,  new binaryExpr, precedence::EQUALITY);

		addInfix(TOKEN_LESS,			new binaryExpr, precedence::COMPARISON);
		addInfix(TOKEN_LESS_EQUAL,		new binaryExpr, precedence::COMPARISON);
		addInfix(TOKEN_GREATER,			new binaryExpr, precedence::COMPARISON);
		addInfix(TOKEN_GREATER_EQUAL,	new binaryExpr, precedence::COMPARISON);

		addInfix(TOKEN_BITSHIFT_LEFT,	new binaryExpr, precedence::BITSHIFT);
		addInfix(TOKEN_BITSHIFT_RIGHT,	new binaryExpr, precedence::BITSHIFT);

		addInfix(TOKEN_PLUS,	new binaryExpr, precedence::SUM);
		addInfix(TOKEN_MINUS,	new binaryExpr, precedence::SUM);

		addInfix(TOKEN_SLASH,		new binaryExpr, precedence::FACTOR);
		addInfix(TOKEN_STAR,		new binaryExpr, precedence::FACTOR);
		addInfix(TOKEN_PERCENTAGE,	new binaryExpr, precedence::FACTOR);

		addInfix(TOKEN_LEFT_PAREN,		new callExpr, precedence::CALL);
		addInfix(TOKEN_LEFT_BRACKET,	new fieldAccessExpr, precedence::CALL);
		addInfix(TOKEN_DOT,				new fieldAccessExpr, precedence::CALL);

		addInfix(TOKEN_DOUBLE_COLON,	new binaryExpr, precedence::PRIMARY);

		//Postfix
		//postfix and mixfix operators get parsed with the infix parselets
		addInfix(TOKEN_INCREMENT, new unaryVarAlterPostfix, precedence::ALTER);
		addInfix(TOKEN_DECREMENT, new unaryVarAlterPostfix, precedence::ALTER);
	#pragma endregion

	#ifdef DEBUG_PRINT_AST
		//debugASTPrinter printer(statements);
	#endif // DEBUG_PRINT_AST
}

parser::~parser() {
	for (auto it : prefixParselets) delete it.second;
	for (auto it : infixParselets) delete it.second;
}

vector<translationUnit*> parser::parse(string path, string name) {
	preprocessor pp(path, name);
	vector<preprocessUnit*> sortedUnits = pp.getSortedUnits();
	tracker = pp.tracker;
	if (pp.hadError) hadError = true;
	for (preprocessUnit* pUnit : sortedUnits) {
		translationUnit* unit = new translationUnit(units, pUnit);
		units.push_back(unit);
		curUnit = unit;
		reset(pUnit->tokens);
		if (tokens.empty()) hadError = true;
		else {
			while (!isAtEnd()) {
				try {
					unit->stmts.push_back(declaration());
				}
				catch (int e) {
					sync();
				}
			}
		}
	}
	return units;
}

ASTNode* parser::expression(int prec) {
	Token token = advance();
	//check if the token has a prefix function associated with it, if it does, parse with it
	if (prefixParselets.count(token.type) == 0) {
		throw error(token, "Expected expression.");
	}
	prefixParselet* prefix = prefixParselets[token.type];
	ASTNode* left = prefix->parse(token);

	//only compiles if the next token has a higher associativity than the current one
	//eg. 1 + 2 compiles because the base prec is 0, and '+' is 9
	//while loop handles continous use of ops, eg. 1 + 2 * 3
	while (prec < getPrec()) {
		token = advance();
		if (infixParselets.count(token.type) == 0) {
			throw error(token, "Expected expression.");
		}
		infixParselet* infix = infixParselets[token.type];
		left = infix->parse(left, token, prec);
	}
	return left;
}

ASTNode* parser::expression() {
	return expression(0);
}

#pragma region Statements and declarations
ASTNode* parser::declaration() {
	if (match({ TOKEN_VAR })) return varDecl();
	else if (match({ TOKEN_CLASS })) return classDecl();
	else if (match({ TOKEN_FUNC })) return funcDecl();
	return statement();
}

ASTNode* parser::varDecl() {
	Token name = consume(TOKEN_IDENTIFIER, "Expected a variable identifier.");
	ASTNode* expr = NULL;
	if (match({ TOKEN_EQUAL })) {
		expr = expression();
	}
	consume(TOKEN_SEMICOLON, "Expected a ; after variable declaration.");
	return new ASTVarDecl(name, expr);
}

ASTNode* parser::funcDecl() {
	//the depths are used for throwing errors for switch and loops stmts, and since a function can be declared inside a loop we need to account
	//for that
	int tempLoopDepth = loopDepth;
	loopDepth = 0;
	int tempSwitchDepth = switchDepth;
	switchDepth = 0;

	Token name = consume(TOKEN_IDENTIFIER, "Expected a function name.");
	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	vector<Token> args;
	int arity = 0;
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			Token arg = consume(TOKEN_IDENTIFIER, "Expect argument name");
			args.push_back(arg);
			arity++;
			if (arity > 255) {
				throw error(arg, "Functions can't have more than 255 arguments");
			}
		} while (match({ TOKEN_COMMA }));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
	consume(TOKEN_LEFT_BRACE, "Expect '{' after arguments.");
	ASTNode* body = blockStmt();

	loopDepth = tempLoopDepth;
	switchDepth = tempSwitchDepth;
	return new ASTFunc(name, args, arity, body);
}

ASTNode* parser::classDecl() {
	Token name = consume(TOKEN_IDENTIFIER, "Expected a class name.");
	Token inherited;
	//inheritance is optional
	if (match({ TOKEN_COLON })) inherited = consume(TOKEN_IDENTIFIER, "Expected a parent class name.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
	//a class body can contain only methods(fields are initialized in the constructor
	vector<ASTNode*> methods;
	while (!check(TOKEN_RIGHT_BRACE) && !isAtEnd()) {
		methods.push_back(funcDecl());
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
	return new ASTClass(name, methods, inherited, inherited.line != -1);
}

ASTNode* parser::statement() {
	if (match({ TOKEN_PRINT, TOKEN_LEFT_BRACE, TOKEN_IF, TOKEN_WHILE, TOKEN_FOR, TOKEN_BREAK, TOKEN_SWITCH, TOKEN_RETURN, TOKEN_CONTINUE})) {
		switch (previous().type) {
		case TOKEN_PRINT: return printStmt();
		case TOKEN_LEFT_BRACE: return blockStmt();
		case TOKEN_IF: return ifStmt();
		case TOKEN_WHILE: return whileStmt();
		case TOKEN_FOR: return forStmt();
		case TOKEN_BREAK: return breakStmt();
		case TOKEN_CONTINUE: return continueStmt();
		case TOKEN_SWITCH: return switchStmt();
		case TOKEN_RETURN: return _return();
		}
	}
	return exprStmt();
}

ASTNode* parser::printStmt() {
	ASTNode* expr = expression();
	consume(TOKEN_SEMICOLON, "Expected ; after expression.");
	return new ASTPrintStmt(expr);
}

ASTNode* parser::exprStmt() {
	ASTNode* expr = expression();
	consume(TOKEN_SEMICOLON, "Expected ; after expression.");
	return new ASTExprStmt(expr);
}

ASTNode* parser::blockStmt() {
	vector<ASTNode*> stmts;
	while (!check(TOKEN_RIGHT_BRACE)) {
		stmts.push_back(declaration());
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
	return new ASTBlockStmt(stmts);
}

ASTNode* parser::ifStmt() {
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	ASTNode* condition =  expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	//using statement() instead of declaration() disallows declarations directly in a control flow body
	ASTNode* thenBranch = statement();
	ASTNode* elseBranch = NULL;
	if (match({ TOKEN_ELSE })) {
		elseBranch = statement();
	}
	return new ASTIfStmt(thenBranch, elseBranch, condition);
}

ASTNode* parser::whileStmt() {
	loopDepth++;
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	ASTNode* condition = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	ASTNode* body = statement();
	loopDepth--;
	return new ASTWhileStmt(body, condition);
}

ASTNode* parser::forStmt() {
	loopDepth++;
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	//initalizer
	ASTNode* init = NULL;
	if (match({ TOKEN_SEMICOLON })) {
		//do nothing
	}else if (match({ TOKEN_VAR })) {//this is for differentiating between for and foreach(since they both use the same keyword)
		if (peekNext().type == TOKEN_COLON) {
			return foreachStmt();
		}else init = varDecl();
	}else {
		init = exprStmt();
	}
	//condition
	ASTNode* condition = NULL;
	//we don't want to use exprStmt() because it emits OP_POP, and we'll need the value to determine whether to jump
	if (!match({ TOKEN_SEMICOLON })) condition = expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");
	//increment
	ASTNode* increment = NULL;
	//using expression() here instead of exprStmt() because there is no trailing ';'
	if (!check(TOKEN_RIGHT_PAREN)) increment = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
	//disallows declarations unless they're in a block
	ASTNode* body = statement();
	loopDepth--;
	return new ASTForStmt(init, condition, increment, body);
}

ASTNode* parser::foreachStmt() {
	loopDepth++;
	Token varName = consume(TOKEN_IDENTIFIER, "Expected a variable name.");
	advance();
	ASTNode* collection = expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after foreach clause.");
	ASTNode* body = statement();
	loopDepth--;
	return new ASTForeachStmt(varName, collection, body);
}

ASTNode* parser::breakStmt() {
	if (loopDepth == 0 && switchDepth == 0) throw error(previous(), "Cannot use 'break' outside of loops or switch statements.");
	consume(TOKEN_SEMICOLON, "Expect ';' after break.");
	return new ASTBreakStmt(previous());
}

ASTNode* parser::continueStmt() {
	if (loopDepth == 0) throw error(previous(), "Cannot use 'continue' outside of loops.");
	consume(TOKEN_SEMICOLON, "Expect ';' after continue.");
	return new ASTContinueStmt(previous());
}

ASTNode* parser::switchStmt() {
	consume(TOKEN_LEFT_PAREN, "Expect '(' after switch.");
	ASTNode* expr = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' after switch expression.");
	switchDepth++;
	vector<ASTNode*> cases;
	bool allStrings = true;
	bool allNum = true;
	bool hasDefault = false;

	while (!check(TOKEN_RIGHT_BRACE) && match({ TOKEN_CASE, TOKEN_DEFAULT })) {
		Token prev = previous();//to see if it's a default statement
		ASTCase* curCase = (ASTCase*)_case();
		if (curCase->getCaseType() == TOKEN_NUMBER) allStrings = false;
		else if (curCase->getCaseType() == TOKEN_STRING) allNum = false;
		else {
			allNum = false;
			allStrings = false;
		}
		if (prev.type == TOKEN_DEFAULT) {
			if (hasDefault) error(prev, "Only 1 default case is allowed inside a switch statement.");
			curCase->setDef(true);
			hasDefault = true;
		}
		cases.push_back(curCase);
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after switch body.");
	switchDepth--;
	return new ASTSwitchStmt(expr, cases, allNum ? switchType::NUM : allStrings ? switchType::STRING : switchType::MIXED, hasDefault);
}

ASTNode* parser::_case() {
	ASTNode* matchExpr = NULL;
	if (previous().type != TOKEN_DEFAULT) {
		matchExpr = expression((int)precedence::PRIMARY);
		if (matchExpr->type == ASTType::GROUPING) throw error(previous(), "Cannot have a grouping expression as a match expression in case.");
	}
	consume(TOKEN_COLON, "Expect ':' after case.");
	vector<ASTNode*> stmts;
	while (!check(TOKEN_CASE) && !check(TOKEN_RIGHT_BRACE) && !check(TOKEN_DEFAULT)) {
		stmts.push_back(statement());
	}
	return new ASTCase(matchExpr, stmts, false);
}

ASTNode* parser::_return() {
	ASTNode* expr = NULL;
	Token keyword = previous();
	if (!match({ TOKEN_SEMICOLON })) {
		expr = expression();
		consume(TOKEN_SEMICOLON, "Expect ';' at the end of return.");
	}
	return new ASTReturn(expr, previous());
}

#pragma endregion

#pragma region Helpers
bool parser::match(const std::initializer_list<TokenType>& tokenTypes) {
	for (TokenType type : tokenTypes) {
		if (check(type)) {
			advance();
			return true;
		}
	}
	return false;
}

bool parser::isAtEnd() {
	return tokens[current].type == TOKEN_EOF;
}

bool parser::check(TokenType type) {
	if (isAtEnd()) return false;
	return peek().type == type;
}

Token parser::advance() {
	if (!isAtEnd()) current++;
	return previous();
}

Token parser::peek() {
	return tokens[current];
}

Token parser::peekNext() {
	return tokens[current + 1];
}

Token parser::previous() {
	return tokens[current - 1];
}

Token parser::consume(TokenType type, string msg) {
	if (check(type)) return advance();

	throw error(peek(), msg);
}

int parser::error(Token token, string msg) {
	hadError = true;
	tracker.addIssue(token, msg, curUnit->src);
	return 0;
}

//syncs when we find a ';' or one of the reserved words
void parser::sync() {
	advance();

	while (!isAtEnd()) {
		if (previous().type == TOKEN_SEMICOLON) return;

		switch (peek().type) {
		case TOKEN_CLASS:
		case TOKEN_FUNC:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
		case TOKEN_SWITCH:
		case TOKEN_RIGHT_BRACE:
			return;
		}

		advance();
	}
}

//parses the args and wraps the callee and args into a call expr
ASTCallExpr* parser::finishCall(ASTNode* callee) {
	Token prev = previous();
	vector<ASTNode*> args;

	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			args.push_back(expression(0));
		} while (match({ TOKEN_COMMA }));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

	return new ASTCallExpr(callee, prev, args);
}

void parser::addPrefix(TokenType type, prefixParselet* parselet, precedence prec) {
	parselet->cur = this;
	parselet->prec = (int)prec;
	prefixParselets.insert_or_assign(type, parselet);
}

void parser::addInfix(TokenType type, infixParselet* parselet, precedence prec) {
	parselet->cur = this;
	parselet->prec = (int)prec;
	infixParselets.insert_or_assign(type, parselet);
}


int parser::getPrec() {
	Token token = peek();
	if (infixParselets.count(token.type) == 0) {
		return 0;
	}
	return infixParselets[token.type]->prec;
}

//called for every new unit we're compiling
void parser::reset(vector<Token> _tokens) {
	tokens = _tokens;
	current = 0;
	loopDepth = 0;
	switchDepth = 0;
}

#pragma endregion
