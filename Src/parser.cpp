#include "parser.h"
#include "AST.h"
#include "debug.h"

#pragma region Parselets

class assignmentExpr : public infixParselet {
	ASTNode* parse(ASTNode* left, Token token) {
		ASTNode* right = cur->expression(prec - 1);
		if (left->type != ASTType::LITERAL && ((ASTLiteralExpr*)left)->getToken().type != TOKEN_IDENTIFIER) {
			throw cur->error(token, "Left side is not assignable");
		}
		return new ASTAssignmentExpr(((ASTLiteralExpr*)left)->getToken(), right);
	}
};

class unaryExpr : public prefixParselet {
	ASTNode* parse(Token token) {
		ASTNode* expr = cur->expression(prec);
		return new ASTUnaryExpr(token, expr);
	}
};

class arrayExpr : public prefixParselet {
	ASTNode* parse(Token token) {
		vector<ASTNode*> members;
		do {
			members.push_back(cur->expression());
		} while (cur->match({ TOKEN_COMMA }));
		cur->consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array initialization.");
		return new ASTArrayDeclExpr(members);
	}
};

class literalExpr : public prefixParselet {
	ASTNode* parse(Token token) {
		if (token.type == TOKEN_LEFT_PAREN) {
			ASTNode* expr = cur->expression();
			cur->consume(TOKEN_RIGHT_PAREN, "Expected ')' at the end of grouping expression.");
			return new ASTGroupingExpr(expr);
		}
		return new ASTLiteralExpr(token);
	}
};

class binaryExpr : public infixParselet {
	ASTNode* parse(ASTNode* left, Token token) {
		ASTNode* right = cur->expression(prec);
		return new ASTBinaryExpr(left, token, right);
	}
};

class callExpr : public infixParselet {
	ASTNode* parse(ASTNode* left, Token token) {
		vector<ASTNode*> args;
		if(token.type == TOKEN_LEFT_PAREN) {
			do {
				args.push_back(cur->expression());
			} while (cur->match({ TOKEN_COMMA }));
			cur->consume(TOKEN_RIGHT_PAREN, "Expect ')' after call expression.");
			return new ASTCallExpr(left, token, args);
		}
		else if (token.type == TOKEN_LEFT_BRACKET) {
			args.push_back(cur->expression());
			cur->consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array/map access.");
		}
		else if (token.type == TOKEN_DOT) {
			args.push_back(cur->expression((int)precedence::PRIMARY));
		}
		if (cur->match({ TOKEN_EQUAL })) {
			ASTNode* val = cur->expression();
			return new ASTSetExpr(left, args[0], token, val);
		}
		return new ASTCallExpr(left, token, args);
	}
};

#pragma endregion

parser::parser(vector<Token>* _tokens) {
	tokens = _tokens;
	current = 0;
	hadError = false;
	scopeDepth = 0;
	loopDepth = 0;
	switchDepth = 0;
	#pragma region Parselets
		//Prefix
		addPrefix(TOKEN_BANG, new unaryExpr, precedence::NOT);
		addPrefix(TOKEN_MINUS, new unaryExpr, precedence::NOT);
		addPrefix(TOKEN_TILDA, new unaryExpr, precedence::NOT);

		addPrefix(TOKEN_IDENTIFIER, new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_STRING, new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_NUMBER, new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_TRUE, new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_FALSE, new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_NIL, new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_LEFT_PAREN, new literalExpr, precedence::PRIMARY);
		addPrefix(TOKEN_LEFT_BRACKET, new arrayExpr, precedence::PRIMARY);

		//Infix
		addInfix(TOKEN_EQUAL, new assignmentExpr, precedence::ASSIGNMENT);

		addInfix(TOKEN_OR, new binaryExpr, precedence::OR);
		addInfix(TOKEN_AND, new binaryExpr, precedence::AND);

		addInfix(TOKEN_BITWISE_OR, new binaryExpr, precedence::BIN_OR);
		addInfix(TOKEN_BITWISE_XOR, new binaryExpr, precedence::BIN_XOR);
		addInfix(TOKEN_BITWISE_AND, new binaryExpr, precedence::BIN_AND);

		addInfix(TOKEN_EQUAL_EQUAL, new binaryExpr, precedence::EQUALITY);
		addInfix(TOKEN_BANG_EQUAL, new binaryExpr, precedence::EQUALITY);

		addInfix(TOKEN_LESS, new binaryExpr, precedence::COMPARISON);
		addInfix(TOKEN_LESS_EQUAL, new binaryExpr, precedence::COMPARISON);
		addInfix(TOKEN_GREATER, new binaryExpr, precedence::COMPARISON);
		addInfix(TOKEN_GREATER_EQUAL, new binaryExpr, precedence::COMPARISON);

		addInfix(TOKEN_BITSHIFT_LEFT, new binaryExpr, precedence::BITSHIFT);
		addInfix(TOKEN_BITSHIFT_RIGHT, new binaryExpr, precedence::BITSHIFT);

		addInfix(TOKEN_PLUS, new binaryExpr, precedence::SUM);
		addInfix(TOKEN_MINUS, new binaryExpr, precedence::SUM);

		addInfix(TOKEN_SLASH, new binaryExpr, precedence::FACTOR);
		addInfix(TOKEN_STAR, new binaryExpr, precedence::FACTOR);
		addInfix(TOKEN_PERCENTAGE, new binaryExpr, precedence::FACTOR);

		addInfix(TOKEN_LEFT_PAREN, new callExpr, precedence::CALL);
		addInfix(TOKEN_LEFT_BRACKET, new callExpr, precedence::CALL);
	#pragma endregion


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
	for (std::map<TokenType, prefixParselet*>::iterator it = prefixParselets.begin(); it != prefixParselets.end(); ++it) delete it->second;
	for (std::map<TokenType, infixParselet*>::iterator it = infixParselets.begin(); it != infixParselets.end(); ++it) delete it->second;
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
	if (!match({ TOKEN_SEMICOLON })) {
		expr = expression();
		consume(TOKEN_SEMICOLON, "Expect ';' at the end of return.");
	}
	return new ASTReturn(expr);
}

#pragma endregion

ASTNode* parser::expression(int prec) {
	Token token = advance();
	if (prefixParselets.count(token.type) == 0) {
		throw error(token, "Expected expression.");
	}
	prefixParselet* prefix = prefixParselets[token.type];
	ASTNode* left = prefix->parse(token);

	while (prec < getPrec()) {
		token = advance();

		if (infixParselets.count(token.type) == 0) {
			throw error(token, "Expected expression.");
		}
		infixParselet* infix = infixParselets[token.type];
		left = infix->parse(left, token);
	}
	return left;
}

ASTNode* parser::expression() {
	return expression(0);
}

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

#pragma endregion
