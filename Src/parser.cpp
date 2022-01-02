#include "parser.h"
#include "AST.h"
#include "debug.h"

parser::parser(vector<Token>* _tokens) {
	tokens = _tokens;
	current = 0;
	hadError = false;

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

ASTNode* parser::statement() {
	if (match({ TOKEN_PRINT, TOKEN_LEFT_BRACE, TOKEN_IF, TOKEN_WHILE, TOKEN_FOR })) {
		switch (previous().type) {
		case TOKEN_PRINT: return printStmt();
		case TOKEN_LEFT_BRACE: return blockStmt();
		case TOKEN_IF: return ifStmt();
		case TOKEN_WHILE: return whileStmt();
		case TOKEN_FOR: return forStmt();
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
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	ASTNode* condition = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	ASTNode* body = statement();
	return new ASTWhileStmt(body, condition);
}

ASTNode* parser::forStmt() {
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	//initalizer
	ASTNode* init = NULL;
	if (match({ TOKEN_SEMICOLON })) {
		//do nothing
	}else if (match({ TOKEN_VAR })) {
		init = declaration();
	}else {
		init = exprStmt();
	}
	//condition
	ASTNode* condition = NULL;
	//we don't want to use exprStmt() because it emits OP_POP, and we'll need the value to determine whether to jump
	if (!match({ TOKEN_SEMICOLON })) condition = expression();
	//increment
	ASTNode* increment = NULL;
	//using expression() here instead of exprStmt() because there is no trailing ';'
	if (!check(TOKEN_RIGHT_PAREN)) increment = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	//disallows declarations unless they're in a block
	ASTNode* body = statement();
	return new ASTForStmt(init, condition, increment, body);
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
	while(match({ TOKEN_MINUS, TOKEN_BANG, TOKEN_TILDA })) {
		Token op = previous();
		ASTNode* right = primary();
		return new ASTUnaryExpr(op, right);
	}
	return primary();
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

#pragma endregion
