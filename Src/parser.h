#ifndef __IFS_PARSER
#define __IFS_PARSER
#include "common.h"
#include "AST.h"
#include "scanner.h"
#include <initializer_list>

class parser {
public:
	parser(vector<Token>* _tokens);
	~parser();
	ASTNode* tree;
	bool hadError;
private:
	vector<Token>* tokens;
	uint16_t current;
#pragma region Precedence

	ASTNode* expression();

	ASTNode* equality();

	ASTNode* comparison();

	ASTNode* bitshift();

	ASTNode* term();

	ASTNode* factor();

	ASTNode* unary();

	ASTNode* primary();

#pragma endregion

#pragma region Helpers

	bool match(const std::initializer_list<TokenType> &tokenTypes);

	bool isAtEnd();

	bool check(TokenType type);

	Token advance();

	Token peek();

	Token previous();

	Token consume(TokenType type, string msg);

	int error(Token token, string msg);

	void report(int line, string _where, string msg);

	void sync();

#pragma endregion
};


#endif // !__IFS_PARSER
