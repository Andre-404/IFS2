#include "common.h"
#include "scanner.h"

class ASTBinaryExpr;
class ASTUnaryExpr;
class ASTGroupingExpr;
class ASTLiteralExpr;

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
	ASTBinaryExpr(Token _op, ASTNode* _left, ASTNode* _right);
	void accept(visitor* vis);
};

class ASTUnaryExpr : public ASTNode {
private:
	Token op;
	ASTNode* right;
public:
	ASTUnaryExpr(Token _op, ASTNode* _right);
	void accept(visitor* vis);
};

class ASTGroupingExpr : public ASTNode {
private:
	ASTNode* expr;
public:
	ASTGroupingExpr(ASTNode* _expr);
	void accept(visitor* vis);
};

class ASTLiteralExpr : public ASTNode {
private:
	Token token;
public:
	ASTLiteralExpr(Token _token);
	void accept(visitor* vis);
};