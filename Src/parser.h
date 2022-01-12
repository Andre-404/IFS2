#ifndef __IFS_PARSER
#define __IFS_PARSER
#include "common.h"
#include "AST.h"
#include "scanner.h"
#include <initializer_list>
#include <map>

class parser;

enum class precedence {
	NONE,
	ASSIGNMENT,
	CONDITIONAL,
	OR,
	AND,
	BIN_OR,
	BIN_XOR,
	BIN_AND,
	EQUALITY,
	COMPARISON,
	BITSHIFT,
	SUM,
	FACTOR,
	NOT,
	CALL,
	PRIMARY
};

class prefixParselet {
public:
	virtual ASTNode* parse(Token token) = 0;
	parser* cur;
	int prec;
};

class infixParselet {
public:
	virtual ASTNode* parse(ASTNode* left, Token token) = 0;
	parser* cur;
	int prec;
};

class parser {
public:
	parser(vector<Token>* _tokens);
	~parser();
	vector<ASTNode*> statements;
	bool hadError;

	ASTNode* expression(int prec);
	ASTNode* expression();
	#pragma region Helpers

		bool match(const std::initializer_list<TokenType>& tokenTypes);

		bool isAtEnd();

		bool check(TokenType type);

		Token advance();

		Token peek();

		Token previous();

		Token consume(TokenType type, string msg);

		int error(Token token, string msg);

		void report(int line, string _where, string msg);

		void sync();

		ASTCallExpr* finishCall(ASTNode* callee);

		int getPrec();

	#pragma endregion
private:
	vector<Token>* tokens;
	uint16_t current;
	int scopeDepth;
	int loopDepth;
	int switchDepth;
	std::map<TokenType, prefixParselet*> prefixParselets;
	std::map<TokenType, infixParselet*> infixParselets;

	void addPrefix(TokenType type, prefixParselet* parselet, precedence prec);
	void addInfix(TokenType type, infixParselet* parselet, precedence prec);

#pragma region Statements
	ASTNode* declaration();
	ASTNode* varDecl();
	ASTNode* funcDecl();
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
	ASTNode* _return();
#pragma endregion

#pragma region Precedence

	

	/*ASTNode* assignment();

	ASTNode* _or();

	ASTNode* _and();

	ASTNode* equality();

	ASTNode* comparison();

	ASTNode* bitshift();

	ASTNode* term();

	ASTNode* factor();

	ASTNode* unary();

	ASTNode* arrayDecl();

	ASTNode* call();

	ASTNode* primary();*/

#pragma endregion

};


#endif // !__IFS_PARSER
