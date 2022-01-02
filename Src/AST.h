#ifndef __IFS_AST
#define __IFS_AST

#include "common.h"
#include "scanner.h"

class ASTAssignmentExpr;
class ASTOrExpr;
class ASTAndExpr;
class ASTBinaryExpr;
class ASTUnaryExpr;
class ASTGroupingExpr;
class ASTLiteralExpr;

class ASTVarDecl;

class ASTPrintStmt;
class ASTExprStmt;
class ASTBlockStmt;
class ASTIfStmt;
class ASTWhileStmt;
class ASTForStmt;
class ASTBreakStmt;

enum class ASTType {
	ASSINGMENT,
	AND,
	OR,
	BINARY,
	UNARY,
	GROUPING,
	LITERAL,
	VAR_DECL,
	PRINT_STMT,
	EXPR_STMT,
	BLOCK_STMT,
	IF_STMT,
	WHILE_STMT,
	FOR_STMT,
	BREAK_STMT
};


//visitor pattern
class visitor {
public:
	virtual ~visitor() {};
	virtual void visitAssignmentExpr(ASTAssignmentExpr* expr) = 0;
	virtual void visitOrExpr(ASTOrExpr* expr) = 0;
	virtual void visitAndExpr(ASTAndExpr* expr) = 0;
	virtual void visitBinaryExpr(ASTBinaryExpr* expr) = 0;
	virtual void visitUnaryExpr(ASTUnaryExpr* expr) = 0;
	virtual void visitGroupingExpr(ASTGroupingExpr* expr) = 0;
	virtual void visitLiteralExpr(ASTLiteralExpr* expr) = 0;

	virtual void visitVarDecl(ASTVarDecl* stmt) = 0;

	virtual void visitPrintStmt(ASTPrintStmt* stmt) = 0;
	virtual void visitExprStmt(ASTExprStmt* stmt) = 0;
	virtual void visitBlockStmt(ASTBlockStmt* stmt) = 0;
	virtual void visitIfStmt(ASTIfStmt* stmt) = 0;
	virtual void visitWhileStmt(ASTWhileStmt* stmt) = 0;
	virtual void visitForStmt(ASTForStmt* stmt) = 0;
	virtual void visitBreakStmt(ASTBreakStmt* stmt) = 0;
};

class ASTNode {
public:
	ASTType type;
	virtual ~ASTNode() {};
	virtual void accept(visitor* vis) = 0;
};

#pragma region Expressions

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

class ASTOrExpr : public ASTNode {
private:
	ASTNode* left;
	ASTNode* right;
public:
	ASTOrExpr(ASTNode* _left, ASTNode* _right);
	~ASTOrExpr();
	void accept(visitor* vis);

	ASTNode* getLeft() { return left; }
	ASTNode* getRight() { return right; }
};

class ASTAndExpr : public ASTNode {
private:
	ASTNode* left;
	ASTNode* right;
public:
	ASTAndExpr(ASTNode* _left, ASTNode* _right);
	~ASTAndExpr();
	void accept(visitor* vis);

	ASTNode* getLeft() { return left; }
	ASTNode* getRight() { return right; }
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
#pragma endregion

#pragma region Statements

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

class ASTBlockStmt : public ASTNode {
private:
	vector<ASTNode*> statements;
public:
	ASTBlockStmt(vector<ASTNode*>& _statements);
	~ASTBlockStmt();
	void accept(visitor* vis);
	vector<ASTNode*> getStmts() { return statements; }
};

class ASTIfStmt : public ASTNode {
private:
	ASTNode* thenBranch;
	ASTNode* elseBranch;
	ASTNode* condition;
public:
	ASTIfStmt(ASTNode* _then, ASTNode* _else, ASTNode* _condition);
	~ASTIfStmt();
	void accept(visitor* vis);
	ASTNode* getCondition() { return condition; }
	ASTNode* getThen() { return thenBranch; }
	ASTNode* getElse() { return elseBranch; }
};

class ASTWhileStmt : public ASTNode {
private:
	ASTNode* body;
	ASTNode* condition;
public:
	ASTWhileStmt(ASTNode* _body, ASTNode* _condition);
	~ASTWhileStmt();
	void accept(visitor* vis);
	ASTNode* getCondition() { return condition; }
	ASTNode* getBody() { return body; }
};

class ASTForStmt : public ASTNode {
private:
	ASTNode* body;
	ASTNode* init;
	ASTNode* condition;
	ASTNode* increment;
public:
	ASTForStmt(ASTNode* _init, ASTNode* _condition, ASTNode* _increment, ASTNode* _body);
	~ASTForStmt();
	void accept(visitor* vis);
	ASTNode* getCondition() { return condition; }
	ASTNode* getBody() { return body; }
	ASTNode* getInit() { return init; }
	ASTNode* getIncrement() { return increment; }
};

class ASTBreakStmt : public ASTNode {
private:
	Token token;
public:
	ASTBreakStmt(Token _token);
	void accept(visitor* vis);
	Token getToken() { return token; }
};

#pragma endregion

#endif