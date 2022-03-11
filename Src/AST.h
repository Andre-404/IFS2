#pragma once

#include "common.h"
#include "scanner.h"

class ASTAssignmentExpr;
class ASTSetExpr;
class ASTConditionalExpr;
class ASTBinaryExpr;
class ASTUnaryExpr;
class ASTArrayDeclExpr;
class ASTCallExpr;
class ASTGroupingExpr;
class ASTUnaryVarAlterExpr;
class ASTStructLiteral;
class ASTLiteralExpr;
class ASTYieldExpr;
class ASTFiberRunExpr;
class ASTFiberLiteral;
class ASTSuperExpr;

class ASTVarDecl;
class ASTFunc;
class ASTClass;

class ASTPrintStmt;
class ASTExprStmt;
class ASTBlockStmt;
class ASTIfStmt;
class ASTWhileStmt;
class ASTForStmt;
class ASTForeachStmt;
class ASTBreakStmt;
class ASTSwitchStmt;
class ASTCase;
class ASTReturn;

enum class ASTType {
	ASSINGMENT,
	SET,
	CONDITIONAL,
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
	YIELD,
	RUN_FIBER,
	FIBER_LITERAL,
	SUPER,
	VAR_DECL,
	PRINT_STMT,
	EXPR_STMT,
	BLOCK_STMT,
	IF_STMT,
	WHILE_STMT,
	FOR_STMT,
	FOREACH_STMT,
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
	virtual void visitConditionalExpr(ASTConditionalExpr* expr) = 0;
	virtual void visitBinaryExpr(ASTBinaryExpr* expr) = 0;
	virtual void visitUnaryExpr(ASTUnaryExpr* expr) = 0;
	virtual void visitUnaryVarAlterExpr(ASTUnaryVarAlterExpr* expr) = 0;
	virtual void visitCallExpr(ASTCallExpr* expr) = 0;
	virtual void visitGroupingExpr(ASTGroupingExpr* expr) = 0;
	virtual void visitArrayDeclExpr(ASTArrayDeclExpr* expr) = 0;
	virtual void visitStructLiteralExpr(ASTStructLiteral* expr) = 0;
	virtual void visitLiteralExpr(ASTLiteralExpr* expr) = 0;
	virtual void visitYieldExpr(ASTYieldExpr* expr) = 0;
	virtual void visitFiberRunExpr(ASTFiberRunExpr* expr) = 0;
	virtual void visitFiberLiteralExpr(ASTFiberLiteral* expr) = 0;
	virtual void visitSuperExpr(ASTSuperExpr* expr) = 0;

	virtual void visitVarDecl(ASTVarDecl* decl) = 0;
	virtual void visitFuncDecl(ASTFunc* decl) = 0;
	virtual void visitClassDecl(ASTClass* decl) = 0;

	virtual void visitPrintStmt(ASTPrintStmt* stmt) = 0;
	virtual void visitExprStmt(ASTExprStmt* stmt) = 0;
	virtual void visitBlockStmt(ASTBlockStmt* stmt) = 0;
	virtual void visitIfStmt(ASTIfStmt* stmt) = 0;
	virtual void visitWhileStmt(ASTWhileStmt* stmt) = 0;
	virtual void visitForStmt(ASTForStmt* stmt) = 0;
	virtual void visitForeachStmt(ASTForeachStmt* stmt) = 0;
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
	void accept(visitor* vis);
	
	ASTNode* getCallee() { return callee; }
	ASTNode* getField() { return field; }
	Token getAccessor() { return accessor; }
	ASTNode* getValue() { return value; }
};

class ASTConditionalExpr : public ASTNode {
private:
	ASTNode* condition;
	ASTNode* thenBranch;
	ASTNode* elseBranch;
public:
	ASTConditionalExpr(ASTNode* _condition, ASTNode* _thenBranch, ASTNode* _elseBranch);
	
	void accept(visitor* vis);
	ASTNode* getCondition() { return condition; }
	ASTNode* getThenBranch() { return thenBranch; }
	ASTNode* getElseBranch() { return elseBranch; }
};

class ASTBinaryExpr : public ASTNode {
private:
	Token op;
	ASTNode* left;
	ASTNode* right;
public:
	ASTBinaryExpr(ASTNode* _left, Token _op, ASTNode* _right);
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
	void accept(visitor* vis);
	ASTNode* getCallee() { return callee; }
	vector<ASTNode*> getArgs() { return args; }
	Token getAccessor() { return accessor; }
};

class ASTSuperExpr : public ASTNode {
private:
	Token methodName;
public:
	ASTSuperExpr(Token _methodName);

	void accept(visitor* vis);
	Token getName() { return methodName; }
};

class ASTGroupingExpr : public ASTNode {
private:
	ASTNode* expr;
public:
	ASTGroupingExpr(ASTNode* _expr);
	void accept(visitor* vis);

	ASTNode* getExpr() { return expr; }
};

class ASTUnaryVarAlterExpr : public ASTNode {
private:
	ASTNode* incrementExpr;
	bool isPositive;
	bool isPrefix;
public:
	ASTUnaryVarAlterExpr(ASTNode* _incrementExpr, bool _isPrefix, bool _isPositive);
	void accept(visitor* vis);

	bool getIsPositive() { return isPositive; }
	ASTNode* getIncrementExpr() { return incrementExpr; }
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

	void accept(visitor* vis);
	vector<structEntry>& getEntries() { return fields; }
};

class ASTYieldExpr : public ASTNode {
private:
	ASTNode* expr;
public:
	ASTYieldExpr(ASTNode* _expr);

	void accept(visitor* vis);

	ASTNode* getExpr() { return expr; }
};

class ASTFiberLiteral : public ASTNode {
private:
	vector<Token> startParams;
	int arity;
	ASTNode* body;
public:
	ASTFiberLiteral(vector<Token>& _startParams, int arity, ASTNode* _body);

	void accept(visitor* vis);

	int getArity() { return arity; }
	vector<Token> getStartParams() { return startParams; }
	ASTNode* getBody() { return body; }
};

class ASTFiberRunExpr : public ASTNode {
private:
	vector<ASTNode*> args;
public:
	ASTFiberRunExpr(vector<ASTNode*>& _args);

	void accept(visitor* vis);

	vector<ASTNode*> getArgs() { return args; }
};

#pragma endregion

#pragma region Statements

class ASTPrintStmt : public ASTNode {
private:
	ASTNode* expr;
public:
	ASTPrintStmt(ASTNode* _expr);
	void accept(visitor* vis);
	ASTNode* getExpr() { return expr; }
};

class ASTExprStmt : public ASTNode {
private :
	ASTNode* expr;
public:
	ASTExprStmt(ASTNode* _expr);
	void accept(visitor* vis);

	ASTNode* getExpr() { return expr; }
};

class ASTVarDecl : public ASTNode {
private: 
	ASTNode* setExpr;
	Token name;
public:
	ASTVarDecl(Token _name, ASTNode* _setExpr);
	void accept(visitor* vis);
	ASTNode* getExpr() { return setExpr; }
	Token getToken() { return name; }
};

class ASTBlockStmt : public ASTNode {
private:
	vector<ASTNode*> statements;
public:
	ASTBlockStmt(vector<ASTNode*>& _statements);
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
	void accept(visitor* vis);
	ASTNode* getCondition() { return condition; }
	ASTNode* getBody() { return body; }
	ASTNode* getInit() { return init; }
	ASTNode* getIncrement() { return increment; }
};

class ASTForeachStmt : public ASTNode {
private:
	Token varName;
	ASTNode* collection;
	ASTNode* body;
public:
	ASTForeachStmt(Token _varName, ASTNode* _collection, ASTNode* _body);
	
	void accept(visitor* vis);
	Token getVarName() { return varName; }
	ASTNode* getCollection() { return collection; }
	ASTNode* getBody() { return body; }
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
	ASTFunc(Token _name, vector<Token>& _args, int _arity, ASTNode* _body);

	void accept(visitor* vis);

	int getArity() { return arity; }
	vector<Token> getArgs() { return args; }
	ASTNode* getBody() { return body; }
	Token getName() { return name; }
};

class ASTReturn : public ASTNode {
private:
	ASTNode* expr;
	//for error reporting
	Token keyword;
public:
	ASTReturn(ASTNode* _expr, Token keyword);
	void accept(visitor* vis);
	ASTNode* getExpr() { return expr; }
	Token getKeyword() { return keyword; }
};

class ASTClass : public ASTNode {
private:
	Token name;
	Token inheritedClass;
	vector<ASTNode*> methods;
	bool _inherits;
public:
	ASTClass(Token _name, vector<ASTNode*> _methods, Token _inheritedClass, bool __inherits);
	void accept(visitor* vis);

	Token getName() { return name; }
	Token getInherited() { return inheritedClass; }
	vector<ASTNode*> getMethods() { return methods; }
	bool inherits() { return _inherits; }
};
#pragma endregion