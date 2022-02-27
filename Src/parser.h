#pragma once
#include "common.h"
#include "AST.h"
#include "scanner.h"
#include <initializer_list>
#include <map>
#include <unordered_map>

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
	ALTER,
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
	virtual ASTNode* parse(ASTNode* left, Token token, int surroundingPrec) = 0;
	parser* cur;
	int prec;
};

class assignmentExpr;
class unaryExpr;
class literalExpr;
class unaryVarAlterPrefix;
class unaryVarAlterPostfix;
class binaryExpr;
class callExpr;
class conditionalExpr;

struct translationUnit {
	vector<ASTNode*> stmts;
	string name;
	translationUnit(string _name) : name(_name) {};
};

class parser {
public:
	parser();
	~parser();
	bool hadError;
	vector<translationUnit*> parse(string path, string name);

	//this is made public so that the parselets can access it
	ASTNode* expression(int prec);
	ASTNode* expression();
	#pragma region Helpers

		bool match(const std::initializer_list<TokenType>& tokenTypes);

		bool isAtEnd();

		bool check(TokenType type);

		Token advance();

		Token peek();

		Token peekNext();

		Token previous();

		Token consume(TokenType type, string msg);

		int error(Token token, string msg);

		void report(int line, string _where, string msg);

		void sync();

		ASTCallExpr* finishCall(ASTNode* callee);

		int getPrec();
	#pragma endregion
private:
	vector<translationUnit*> units;
	translationUnit* curUnit;

	vector<Token> tokens;
	uint16_t current;

	int scopeDepth;
	int loopDepth;
	int switchDepth;

	std::unordered_map<TokenType, prefixParselet*> prefixParselets;
	std::unordered_map<TokenType, infixParselet*> infixParselets;

	void addPrefix(TokenType type, prefixParselet* parselet, precedence prec);
	void addInfix(TokenType type, infixParselet* parselet, precedence prec);

	void reset(vector<Token> tokens);
	

	#pragma region Statements
	void importStmt();
	ASTNode* declaration();
	ASTNode* varDecl();
	ASTNode* funcDecl();
	ASTNode* classDecl();
	ASTNode* statement();

	ASTNode* printStmt();
	ASTNode* exprStmt();
	ASTNode* blockStmt();
	ASTNode* ifStmt();
	ASTNode* whileStmt();
	ASTNode* forStmt();
	ASTNode* foreachStmt();
	ASTNode* breakStmt();
	ASTNode* switchStmt();
	ASTNode* _case();
	ASTNode* _return();

#pragma endregion


};
