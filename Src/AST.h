#ifndef __IFS_AST
#define __IFS_AST

#include "common.h"
#include "scanner.h"

class ASTAssignmentExpr;
class ASTSetExpr;
class ASTBinaryExpr;
class ASTUnaryExpr;
class ASTArrayDeclExpr;
class ASTCallExpr;
class ASTGroupingExpr;
class ASTUnaryVarAlterExpr;
class ASTStructLiteral;
class ASTLiteralExpr;

class ASTVarDecl;
class ASTFunc;
class ASTClass;

class ASTPrintStmt;
class ASTExprStmt;
class ASTBlockStmt;
class ASTIfStmt;
class ASTWhileStmt;
class ASTForStmt;
class ASTBreakStmt;
class ASTSwitchStmt;
class ASTCase;
class ASTReturn;

enum class ASTType {
	ASSINGMENT,
	SET,
	OR,
	AND,
	BINARY,
	UNARY,
	GROUPING,
	ARRAY,
	VAR_ALTER,
	CALL,
	STRUCT_LITERAL,
	LITERAL,
	VAR_DECL,
	PRINT_STMT,
	EXPR_STMT,
	BLOCK_STMT,
	IF_STMT,
	WHILE_STMT,
	FOR_STMT,
	BREAK_STMT,
	SWITCH_STMT,
	CASE,
	FUNC,
	CLASS,
	
	RETURN,
};

enum class switchType {
	MIXED,
	NUM,
	STRING
};


//visitor pattern
class visitor {
public:
	virtual ~visitor() {};
	virtual void visitAssignmentExpr(ASTAssignmentExpr* expr) = 0;
	virtual void visitSetExpr(ASTSetExpr* expr) = 0;
	virtual void visitBinaryExpr(ASTBinaryExpr* expr) = 0;
	virtual void visitUnaryExpr(ASTUnaryExpr* expr) = 0;
	virtual void visitUnaryVarAlterExpr(ASTUnaryVarAlterExpr* expr) = 0;
	virtual void visitCallExpr(ASTCallExpr* expr) = 0;
	virtual void visitGroupingExpr(ASTGroupingExpr* expr) = 0;
	virtual void visitArrayDeclExpr(ASTArrayDeclExpr* expr) = 0;
	virtual void visitStructLiteralExpr(ASTStructLiteral* expr) = 0;
	virtual void visitLiteralExpr(ASTLiteralExpr* expr) = 0;

	virtual void visitVarDecl(ASTVarDecl* decl) = 0;
	virtual void visitFuncDecl(ASTFunc* decl) = 0;
	virtual void visitClassDecl(ASTClass* decl) = 0;

	virtual void visitPrintStmt(ASTPrintStmt* stmt) = 0;
	virtual void visitExprStmt(ASTExprStmt* stmt) = 0;
	virtual void visitBlockStmt(ASTBlockStmt* stmt) = 0;
	virtual void visitIfStmt(ASTIfStmt* stmt) = 0;
	virtual void visitWhileStmt(ASTWhileStmt* stmt) = 0;
	virtual void visitForStmt(ASTForStmt* stmt) = 0;
	virtual void visitBreakStmt(ASTBreakStmt* stmt) = 0;
	virtual void visitSwitchStmt(ASTSwitchStmt* stmt) = 0;
	virtual void visitCase(ASTCase* _case) = 0;
	virtual void visitReturnStmt(ASTReturn* stmt) = 0;
	

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

class ASTSetExpr : public ASTNode{
private:
	ASTNode* callee;
	ASTNode* field;
	Token accessor;
	ASTNode* value;
public:
	ASTSetExpr(ASTNode* _callee, ASTNode* _field, Token _accessor, ASTNode* _val);
	~ASTSetExpr();
	void accept(visitor* vis);
	
	ASTNode* getCallee() { return callee; }
	ASTNode* getField() { return field; }
	Token getAccessor() { return accessor; }
	ASTNode* getValue() { return value; }
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

class ASTArrayDeclExpr : public ASTNode {
private:
	vector<ASTNode*> members;
	int size;
public:
	ASTArrayDeclExpr(vector<ASTNode*>& _members);
	~ASTArrayDeclExpr();
	void accept(visitor* vis);
	vector<ASTNode*> getMembers() { return members; }
	int getSize() { return size; }
};

class ASTCallExpr : public ASTNode {
private:
	ASTNode* callee;
	Token accessor;
	vector<ASTNode*> args;
public:
	ASTCallExpr(ASTNode* _callee, Token _accessor, vector<ASTNode*>& _args);
	~ASTCallExpr();
	void accept(visitor* vis);
	ASTNode* getCallee() { return callee; }
	vector<ASTNode*> getArgs() { return args; }
	Token getAccessor() { return accessor; }
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

class ASTUnaryVarAlterExpr : public ASTNode {
private:
	ASTNode* callee;
	ASTNode* field;
	Token op;
	bool isPrefix;
public:
	ASTUnaryVarAlterExpr(ASTNode* _callee, ASTNode* _field, Token _op, bool _isPrefix);
	~ASTUnaryVarAlterExpr();
	void accept(visitor* vis);

	ASTNode* getCallee() { return callee; }
	ASTNode* getField() { return field; }
	Token getOp() { return op; }
	bool getIsPrefix() { return isPrefix; }
};

class ASTLiteralExpr : public ASTNode {
private:
	Token token;
public:
	ASTLiteralExpr(Token _token);
	void accept(visitor* vis);

	Token getToken() { return token; }
};

struct structEntry {
	ASTNode* expr;
	Token name;
	structEntry(Token _name, ASTNode* _expr) : expr(_expr), name(_name) {};
};

class ASTStructLiteral : public ASTNode {
private:
	vector<structEntry> fields;
public:
	ASTStructLiteral(vector<structEntry> _fields);
	~ASTStructLiteral();

	void accept(visitor* vis);
	vector<structEntry>& getEntries() { return fields; }
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

class ASTSwitchStmt : public ASTNode {
private:
	ASTNode* expr;
	vector<ASTNode*> cases;
	int numCases;
	switchType casesType;
	bool hasDefault;
public:
	ASTSwitchStmt(ASTNode* _expr, vector<ASTNode*>& _cases, switchType _type, bool _hasDefault);
	~ASTSwitchStmt();
	void accept(visitor* vis);
	ASTNode* getExpr() { return expr; }
	vector<ASTNode*> getCases() { return cases; }
	switchType getType() { return casesType; }
};

class ASTCase : public ASTNode {
private:
	ASTNode* expr;
	vector<ASTNode*> stmts;
	TokenType caseType;
	bool isDefault;
public:
	ASTCase(ASTNode* _expr, vector<ASTNode*>& _stmts, bool _isDefault);
	~ASTCase();
	void accept(visitor* vis);
	ASTNode* getExpr() { return expr; }
	vector<ASTNode*> getStmts() { return stmts; }
	TokenType getCaseType() { return caseType; }
	void setDef(bool _newDefault) { isDefault = _newDefault; }
	bool getDef() { return isDefault; }
};

class ASTFunc : public ASTNode {
private:
	vector<Token> args;
	int arity;
	ASTNode* body;
	Token name;
public:
	ASTFunc(Token _name, vector<Token>& _args, int arity, ASTNode* _body);
	~ASTFunc();
	void accept(visitor* vis);
	int getArity() { return arity; }
	vector<Token> getArgs() { return args; }
	ASTNode* getBody() { return body; }
	Token getName() { return name; }
};

class ASTReturn : public ASTNode {
private:
	ASTNode* expr;
public:
	ASTReturn(ASTNode* _expr);
	~ASTReturn();
	void accept(visitor* vis);
	ASTNode* getExpr() { return expr; }
};

class ASTClass : public ASTNode {
private:
	Token name;
	vector<ASTNode*> methods;
public:
	ASTClass(Token _name, vector<ASTNode*> _methods);
	~ASTClass();
	void accept(visitor* vis);

	Token getName() { return name; }
	vector<ASTNode*> getMethods() { return methods; }
};
#pragma endregion

#endif