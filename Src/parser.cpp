#include "parser.h"
#include "AST.h"
#include "debug.h"

parser::parser(vector<Token>* _tokens) {
	tokens = _tokens;
	current = 0;
	hadError = false;

	try {
		tree = expression();
	#ifdef DEBUG_PRINT_AST
		debugASTPrinter printer(tree);
	#endif // DEBUG_PRINT_AST
	}catch (int e) {
		tree = NULL;
	}
}

#pragma region Precedence

/*
when matching tokens we use while to enable multiple operations one after another
*/

ASTNode* parser::expression() {
	return equality();
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
	if (match({ TOKEN_NUMBER, TOKEN_STRING, TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL })) return new ASTLiteralExpr(previous());

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
