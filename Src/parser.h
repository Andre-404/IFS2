#pragma once
#include "common.h"
#include "AST.h"
#include "scanner.h"
#include <initializer_list>
#include <map>
#include <unordered_map>
#include "issueTracker.h"

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

class preprocessUnit;

struct translationUnit {
	vector<ASTNode*> stmts;
	string name;
	file* src;
	//used for figuring out which file a definition is coming from(if the scope resolution operator is used)
	std::vector<translationUnit*> deps;

	translationUnit(vector<translationUnit*>& sortedUnits, preprocessUnit* pUnit);
	~translationUnit() { delete src; }
};

class parser {
public:
	parser();
	~parser();
	bool hadError;
	issueTracker tracker;
	vector<translationUnit*> parse(string path, string name);

private:
	vector<translationUnit*> units;
	translationUnit* curUnit;

	vector<Token> tokens;
	uint16_t current;

	int loopDepth;
	int switchDepth;

	std::unordered_map<TokenType, prefixParselet*> prefixParselets;
	std::unordered_map<TokenType, infixParselet*> infixParselets;

	void addPrefix(TokenType type, prefixParselet* parselet, precedence prec);
	void addInfix(TokenType type, infixParselet* parselet, precedence prec);

	void reset(vector<Token> tokens);

	#pragma region Expressions
	ASTNode* parseAssign(ASTNode* left, Token op);
	ASTNode* expression(int prec);
	ASTNode* expression();

	//doing this mess because friendships isn't inherited
	friend class fiberRunExpr;
	friend class fieldAccessExpr;
	friend class callExpr;
	friend class unaryVarAlterPostfix;
	friend class binaryExpr;
	friend class conditionalExpr;
	friend class assignmentExpr;
	friend class unaryVarAlterPrefix;
	friend class literalExpr;
	friend class unaryExpr;
	#pragma endregion

	#pragma region Statements
	ASTNode* declaration();//had to make this part of the public api because 
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
	ASTNode* continueStmt();
	ASTNode* switchStmt();
	ASTNode* _case();
	ASTNode* _return();

	#pragma endregion

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

	void sync();

	ASTCallExpr* finishCall(ASTNode* callee);

	int getPrec();
	#pragma endregion

};
