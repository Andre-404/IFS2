#ifndef __IFS_AST
#define __IFS_AST

#include "common.h"
#include "scanner.h"

class ASTBinaryExpr;
class ASTUnaryExpr;
class ASTGroupingExpr;
class ASTLiteralExpr;

//visitor pattern
class visitor {
public:
	virtual ~visitor() {};
	virtual void visitBinaryExpr(ASTBinaryExpr* expr) = 0;
	virtual void visitUnaryExpr(ASTUnaryExpr* expr) = 0;
	virtual void visitGroupingExpr(ASTGroupingExpr* expr) = 0;
	virtual void visitLiteralExpr(ASTLiteralExpr* expr) = 0;
};

class ASTNode {
public:
	virtual ~ASTNode() {};
	virtual void accept(visitor* vis) = 0;
};



class ASTBinaryExpr : public ASTNode {
private:
	Token op;
	ASTNode* left;
	ASTNode* right;
public:
	ASTBinaryExpr(ASTNode* _left, Token _op, ASTNode* _right);
	~ASTBinaryExpr();
	void accept(visitor* vis);

	Token getToken() { return op; }
	ASTNode* getLeft() { return left; }
	ASTNode* getRight() { return right; }
};

class ASTUnaryExpr : public ASTNode {
private:
	Token op;
	ASTNode* right;
public:
	ASTUnaryExpr(Token _op, ASTNode* _right);
	~ASTUnaryExpr();
	void accept(visitor* vis);

	Token getToken() { return op; }
	ASTNode* getRight() { return right; }
};

class ASTGroupingExpr : public ASTNode {
private:
	ASTNode* expr;
public:
	ASTGroupingExpr(ASTNode* _expr);
	~ASTGroupingExpr();
	void accept(visitor* vis);

	ASTNode* getExpr() { return expr; }
};

class ASTLiteralExpr : public ASTNode {
private:
	Token token;
public:
	ASTLiteralExpr(Token _token);
	void accept(visitor* vis);

	Token getToken() { return token; }
};

#endif