#ifndef __IFS_AST
#define __IFS_AST

#include "common.h"
#include "scanner.h"

class ASTAssignmentExpr;
class ASTBinaryExpr;
class ASTUnaryExpr;
class ASTGroupingExpr;
class ASTLiteralExpr;

class ASTVarDecl;

class ASTPrintStmt;
class ASTExprStmt;

enum class ASTType{
	ASSINGMENT,
	BINARY,
	UNARY,
	GROUPING,
	LITERAL,
	VAR_DECL,
	PRINT_STMT,
	EXPR_STMT
};


//visitor pattern
class visitor {
public:
	virtual ~visitor() {};
	virtual void visitAssignmentExpr(ASTAssignmentExpr* expr) = 0;
	virtual void visitBinaryExpr(ASTBinaryExpr* expr) = 0;
	virtual void visitUnaryExpr(ASTUnaryExpr* expr) = 0;
	virtual void visitGroupingExpr(ASTGroupingExpr* expr) = 0;
	virtual void visitLiteralExpr(ASTLiteralExpr* expr) = 0;

	virtual void visitVarDecl(ASTVarDecl* stmt) = 0;

	virtual void visitPrintStmt(ASTPrintStmt* stmt) = 0;
	virtual void visitExprStmt(ASTExprStmt* stmt) = 0;
};

class ASTNode {
public:
	ASTType type;
	virtual ~ASTNode() {};
	virtual void accept(visitor* vis) = 0;
};


class ASTAssignmentExpr : public ASTNode {
private:
	Token name;
	ASTNode* value;
public:
	ASTAssignmentExpr(Token _name, ASTNode* _value);
	~ASTAssignmentExpr();
	void accept(visitor* vis);

	Token getToken() { return name; }
	ASTNode* getVal() { return value; }
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

class ASTPrintStmt : public ASTNode {
private:
	ASTNode* expr;
public:
	ASTPrintStmt(ASTNode* _expr);
	~ASTPrintStmt();
	void accept(visitor* vis);
	ASTNode* getExpr() { return expr; }
};

class ASTExprStmt : public ASTNode {
private :
	ASTNode* expr;
public:
	ASTExprStmt(ASTNode* _expr);
	~ASTExprStmt();
	void accept(visitor* vis);

	ASTNode* getExpr() { return expr; }
};

class ASTVarDecl : public ASTNode {
private: 
	ASTNode* setExpr;
	Token name;
public:
	ASTVarDecl(Token _name, ASTNode* _setExpr);
	~ASTVarDecl();
	void accept(visitor* vis);
	ASTNode* getExpr() { return setExpr; }
	Token getToken() { return name; }
};

#endif