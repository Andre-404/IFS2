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
	vector<ASTNode*> statements;
	bool hadError;
private:
	vector<Token>* tokens;
	uint16_t current;
	int scopeDepth;
	int loopDepth;
	int switchDepth;

#pragma region Statements
	ASTNode* declaration();
	ASTNode* varDecl();
	ASTNode* statement();

	ASTNode* printStmt();
	ASTNode* exprStmt();
	ASTNode* blockStmt();
	ASTNode* ifStmt();
	ASTNode* whileStmt();
	ASTNode* forStmt();
	ASTNode* breakStmt();
	ASTNode* switchStmt();
	ASTNode* _case();
#pragma endregion


#pragma region Precedence

	ASTNode* expression();

	ASTNode* assignment();

	ASTNode* _or();

	ASTNode* _and();

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
