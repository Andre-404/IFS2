#include "parser.h"
#include "AST.h"
#include "debug.h"

parser::parser(vector<Token>* _tokens) {
	tokens = _tokens;
	current = 0;
	hadError = false;
	scopeDepth = 0;
	loopDepth = 0;
	switchDepth = 0;

	while (!isAtEnd()) {
		try {
			statements.push_back(declaration());
		}
		catch (int e) {
			sync();
		}
	}
	#ifdef DEBUG_PRINT_AST
		debugASTPrinter printer(statements);
	#endif // DEBUG_PRINT_AST
}

parser::~parser() {
	for (int i = 0; i < statements.size(); i++) {
		delete statements[i];
	}
}

#pragma region Statements and declarations
ASTNode* parser::declaration() {
	if (match({ TOKEN_VAR })) return varDecl();
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
	Token name = consume(TOKEN_IDENTIFIER, "Expected a function name.");
	if (scopeDepth > 0) {
		throw error(name, "Can't declare a function outside of global scope.");
	}
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
	return new ASTFunc(name, args, arity, body);
}

ASTNode* parser::statement() {
	if (match({ TOKEN_PRINT, TOKEN_LEFT_BRACE, TOKEN_IF, TOKEN_WHILE, TOKEN_FOR, TOKEN_BREAK, TOKEN_SWITCH, TOKEN_RETURN})) {
		switch (previous().type) {
		case TOKEN_PRINT: return printStmt();
		case TOKEN_LEFT_BRACE: return blockStmt();
		case TOKEN_IF: return ifStmt();
		case TOKEN_WHILE: return whileStmt();
		case TOKEN_FOR: return forStmt();
		case TOKEN_BREAK: return breakStmt();
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
	scopeDepth++;
	while (!check(TOKEN_RIGHT_BRACE)) {
		stmts.push_back(declaration());
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
	scopeDepth--;
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
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	ASTNode* condition = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	ASTNode* body = statement();
	return new ASTWhileStmt(body, condition);
}

ASTNode* parser::forStmt() {
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	//initalizer
	ASTNode* init = NULL;
	if (match({ TOKEN_SEMICOLON })) {
		//do nothing
	}else if (match({ TOKEN_VAR })) {
		init = varDecl();
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
	return new ASTForStmt(init, condition, increment, body);
}

ASTNode* parser::breakStmt() {
	if (loopDepth == 0 && switchDepth == 0) throw error(previous(), "Cannot use 'break' outside of loops or switch statements.");
	consume(TOKEN_SEMICOLON, "Expect ';' after break.");
	return new ASTBreakStmt(previous());
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
		matchExpr = primary();
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
	if (!match({ TOKEN_SEMICOLON })) {
		expr = expression();
		consume(TOKEN_SEMICOLON, "Expect ';' at the end of return.");
	}
	return new ASTReturn(expr);
}

#pragma endregion

#pragma region Precedence

/*
when matching tokens we use while to enable multiple operations one after another
*/

ASTNode* parser::expression() {
	return assignment();
}

ASTNode* parser::assignment() {
	ASTNode* expr = _or();

	//check if this really is a assigmenent
	if (match({ TOKEN_EQUAL })) {
		Token prev = previous();//for errors
		//get the right side of assingment, can be another assingment
		//eg. a = b = 3;
		ASTNode* val = assignment();

		//check if the left side of the assignment is a literal, possibly containing a variable name
		//(properties of objects get handled differently)
		if (expr->type == ASTType::LITERAL) {
			Token name = ((ASTLiteralExpr*)expr)->getToken();
			//if this really is a variable literal, create a new assingment expr with the variable and the right side expr
			if(name.type == TOKEN_IDENTIFIER) return new ASTAssignmentExpr(name, val);
		}
		error(prev, "Invalid assingment target.");
	}
	return expr;
}

ASTNode* parser::_or() {
	ASTNode* expr = _and();
	while (match({ TOKEN_OR })) {
		ASTNode* right = _and();
		expr = new ASTOrExpr(expr, right);
	}
	return expr;
}

ASTNode* parser::_and() {
	ASTNode* expr = equality();
	while (match({ TOKEN_AND })) {
		ASTNode* right = equality();
		expr = new ASTAndExpr(expr, right);
	}
	return expr;
}

ASTNode* parser::equality() {
	ASTNode* expr = comparison();

	while(match({ TOKEN_EQUAL_EQUAL })) {
		Token op = previous();
		ASTNode* right = comparison();
		expr = new ASTBinaryExpr(expr, op, right);
	}
	return expr;
}

ASTNode* parser::comparison() {
	ASTNode* expr = bitshift();

	while(match({ TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL })) {
		Token op = previous();
		ASTNode* right = bitshift();
		expr = new ASTBinaryExpr(expr, op, right);
	}
	return expr;
}

ASTNode* parser::bitshift() {
	ASTNode* expr = term();

	while(match({ TOKEN_BITSHIFT_LEFT, TOKEN_BITSHIFT_RIGHT })) {
		Token op = previous();
		ASTNode* right = term();
		expr = new ASTBinaryExpr(expr, op, right);
	}
	return expr;
}

ASTNode* parser::term() {
	ASTNode* expr = factor();

	while(match({ TOKEN_PLUS, TOKEN_MINUS })) {
		Token op = previous();
		ASTNode* right = factor();
		expr = new ASTBinaryExpr(expr, op, right);
	}
	return expr;
}

ASTNode* parser::factor() {
	ASTNode* expr = unary();

	while(match({ TOKEN_SLASH, TOKEN_STAR, TOKEN_PERCENTAGE })) {
		Token op = previous();
		ASTNode* right = unary();
		expr = new ASTBinaryExpr(expr, op, right);
	}
	return expr;
}

ASTNode* parser::unary() {
	//TODO: fix this mess
	while(match({ TOKEN_MINUS, TOKEN_BANG, TOKEN_TILDA })) {
		Token op = previous();
		ASTNode* right = call();
		return new ASTUnaryExpr(op, right);
	}
	return call();
}

ASTNode* parser::call() {
	ASTNode* expr = primary();

	while (true) {
		if (match({ TOKEN_LEFT_PAREN })) {
			expr = finishCall(expr);
		}
		else {
			break;
		}
	}

	return expr;
}

ASTNode* parser::primary() {
	if (match({ TOKEN_LEFT_PAREN })) {
		ASTNode* expr = expression();
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
		return new ASTGroupingExpr(expr);
	}
	//and literal is stored as a string_view in the token
	if (match({ TOKEN_NUMBER, TOKEN_STRING, TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL, TOKEN_IDENTIFIER })) return new ASTLiteralExpr(previous());

	throw error(peek(), "Expect expression.");
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
	return tokens->at(current).type == TOKEN_EOF;
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
	return tokens->at(current);
}

Token parser::previous() {
	return tokens->at(current - 1);
}

Token parser::consume(TokenType type, string msg) {
	if (check(type)) return advance();

	throw error(peek(), msg);
}

void parser::report(int line, string _where, string msg) {
	std::cout << "[line " << line << "] Error" << _where << ": " << msg << "\n";
	hadError = true;
}

int parser::error(Token token, string msg) {
	if (token.type == TOKEN_EOF) {
		report(token.line, " at end", msg);
	}
	else {
		//have to do this so we can concat strings(can't concat string_view
		report(token.line, " at '" + string(token.lexeme) + "'", msg);
	}
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
		case TOKEN_FOREACH:
			return;
		}

		advance();
	}
}

ASTCallExpr* parser::finishCall(ASTNode* callee) {
	vector<ASTNode*> args;

	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			args.push_back(expression());
		} while (match({ TOKEN_COMMA }));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

	return new ASTCallExpr(callee, args);
}

#pragma endregion
