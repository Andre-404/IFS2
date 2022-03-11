#include "AST.h"
#include "namespaces.h"

using namespace global;

/*
accept methods invoke the specific visitor functions for each node type
WARNING: don't forget to add 'type' field in the constructor
*/
#pragma region Assigment expr
ASTAssignmentExpr::ASTAssignmentExpr(Token _name, ASTNode* _value) {
	name = _name;
	value = _value;
	type = ASTType::ASSINGMENT;
	gc.addASTNode(this);
}

void ASTAssignmentExpr::accept(visitor* vis) {
	vis->visitAssignmentExpr(this);
}

#pragma endregion

#pragma region Set expr
ASTSetExpr::ASTSetExpr(ASTNode* _callee, ASTNode* _field, Token _accessor, ASTNode* _val) {
	type = ASTType::ARRAY;
	callee = _callee;
	field = _field;
	accessor = _accessor;
	value = _val;
	gc.addASTNode(this);
}

void ASTSetExpr::accept(visitor* vis) {
	vis->visitSetExpr(this);
}


#pragma endregion

#pragma region Conditional expr
ASTConditionalExpr::ASTConditionalExpr(ASTNode* _condition, ASTNode* _thenBranch, ASTNode* _elseBranch) {
	condition = _condition;
	thenBranch = _thenBranch;
	elseBranch = _elseBranch;
	type = ASTType::CONDITIONAL;
	gc.addASTNode(this);
}

void ASTConditionalExpr::accept(visitor* vis) {
	vis->visitConditionalExpr(this);
}
#pragma endregion


#pragma region Binary expr
ASTBinaryExpr::ASTBinaryExpr(ASTNode* _left, Token _op, ASTNode* _right) {
	op = _op;
	left = _left;
	right = _right;
	type = ASTType::BINARY;
	gc.addASTNode(this);
}

void ASTBinaryExpr::accept(visitor* vis) {
	vis->visitBinaryExpr(this);
}
#pragma endregion

#pragma region Unary expr
ASTUnaryExpr::ASTUnaryExpr(Token _op, ASTNode* _right) {
	op = _op;
	right = _right;
	type = ASTType::UNARY;
	gc.addASTNode(this);
}

void ASTUnaryExpr::accept(visitor* vis) {
	vis->visitUnaryExpr(this);
}
#pragma endregion

#pragma region Array decl expr
ASTArrayDeclExpr::ASTArrayDeclExpr(vector<ASTNode*>& _members) {
	members = _members;
	size = members.size();
	type = ASTType::ARRAY;
	gc.addASTNode(this);
}

void ASTArrayDeclExpr::accept(visitor* vis) {
	vis->visitArrayDeclExpr(this);
}

#pragma endregion

#pragma region Call expr
ASTCallExpr::ASTCallExpr(ASTNode* _callee, Token _accessor, vector<ASTNode*>& _args) {
	callee = _callee;
	args = _args;
	type = ASTType::CALL;
	accessor = _accessor;
	gc.addASTNode(this);
}

void ASTCallExpr::accept(visitor* vis) {
	vis->visitCallExpr(this);
}

#pragma endregion

#pragma region Grouping expr
ASTGroupingExpr::ASTGroupingExpr(ASTNode* _expr) {
	expr = _expr;
	ASTType::GROUPING;
	gc.addASTNode(this);
}

void ASTGroupingExpr::accept(visitor* vis) {
	vis->visitGroupingExpr(this);
}
#pragma endregion

#pragma region Unary var altering expr
ASTUnaryVarAlterExpr::ASTUnaryVarAlterExpr(ASTNode* _incrementExpr, bool _isPrefix, bool _isPositive) {
	isPositive = _isPositive;
	incrementExpr = _incrementExpr;
	isPrefix = _isPrefix;
	type = ASTType::VAR_ALTER;
	gc.addASTNode(this);
}

void ASTUnaryVarAlterExpr::accept(visitor* vis) {
	vis->visitUnaryVarAlterExpr(this);
}
#pragma endregion

#pragma region Struct literal
ASTStructLiteral::ASTStructLiteral(vector<structEntry> _fields) {
	fields = _fields;
	type = ASTType::STRUCT_LITERAL;
	gc.addASTNode(this);
}

void ASTStructLiteral::accept(visitor* vis) {
	vis->visitStructLiteralExpr(this);
}
#pragma endregion

#pragma region Literal expr
ASTLiteralExpr::ASTLiteralExpr(Token _token) {
	token = _token;
	type = ASTType::LITERAL;
	gc.addASTNode(this);
}

void ASTLiteralExpr::accept(visitor* vis) {
	vis->visitLiteralExpr(this);
}
#pragma endregion

#pragma region Super literal
ASTSuperExpr::ASTSuperExpr(Token _methodName) {
	type = ASTType::SUPER;
	methodName = _methodName;
	gc.addASTNode(this);
}

void ASTSuperExpr::accept(visitor* vis) {
	vis->visitSuperExpr(this);
}
#pragma endregion

#pragma region Yield expr
ASTYieldExpr::ASTYieldExpr(ASTNode* _expr) {
	expr = _expr;
	gc.addASTNode(this);
}

void ASTYieldExpr::accept(visitor* vis) {
	vis->visitYieldExpr(this);
}


#pragma endregion

#pragma region Fiber literal
ASTFiberLiteral::ASTFiberLiteral(vector<Token>& _startParams, int _arity, ASTNode* _body) {
	startParams = _startParams;
	arity = _arity;
	body = _body;
	gc.addASTNode(this);
}

void ASTFiberLiteral::accept(visitor* vis) {
	vis->visitFiberLiteralExpr(this);
}

#pragma endregion

#pragma region Fiber run
ASTFiberRunExpr::ASTFiberRunExpr(vector<ASTNode*>& _args) {
	args = _args;
	gc.addASTNode(this);
}

void ASTFiberRunExpr::accept(visitor* vis) {
	vis->visitFiberRunExpr(this);
}
#pragma endregion





#pragma region Var decl
ASTVarDecl::ASTVarDecl(Token _name, ASTNode* _setExpr) {
	name = _name;
	setExpr = _setExpr;
	type = ASTType::VAR_DECL;
	gc.addASTNode(this);
}

void ASTVarDecl::accept(visitor* vis) {
	vis->visitVarDecl(this);
}


#pragma endregion

#pragma region Function decl
ASTFunc::ASTFunc(Token _name, vector<Token>& _args, int _arity, ASTNode* _body) {
	args = _args;
	arity = _arity;
	body = _body;
	type = ASTType::FUNC;
	name = _name;
	gc.addASTNode(this);
}

void ASTFunc::accept(visitor* vis) {
	vis->visitFuncDecl(this);
}
#pragma endregion

#pragma region Class decl
ASTClass::ASTClass(Token _name, vector<ASTNode*> _methods, Token _inheritedClassName, bool __inherits) {
	name = _name;
	methods = _methods;
	type = ASTType::CLASS;
	_inherits = __inherits;
	inheritedClass = _inheritedClassName;
	gc.addASTNode(this);
}

void ASTClass::accept(visitor* vis) {
	vis->visitClassDecl(this);
}
#pragma endregion



#pragma region Print stmt
ASTPrintStmt::ASTPrintStmt(ASTNode* _expr) {
	expr = _expr;
	type = ASTType::PRINT_STMT;
	gc.addASTNode(this);
}

void ASTPrintStmt::accept(visitor* vis) {
	vis->visitPrintStmt(this);
}
#pragma endregion

#pragma region Expr stmt
ASTExprStmt::ASTExprStmt(ASTNode* _expr) {
	expr = _expr;
	ASTType::EXPR_STMT;
	gc.addASTNode(this);
}

void ASTExprStmt::accept(visitor* vis) {
	vis->visitExprStmt(this);
}


#pragma endregion

#pragma region Block stmt
ASTBlockStmt::ASTBlockStmt(vector<ASTNode*>& _statements) {
	statements = _statements;
	type = ASTType::BLOCK_STMT;
	gc.addASTNode(this);
}

void ASTBlockStmt::accept(visitor* vis) {
	vis->visitBlockStmt(this);
}


#pragma endregion

#pragma region If stmt
ASTIfStmt::ASTIfStmt(ASTNode* _then, ASTNode* _else, ASTNode* _condition) {
	thenBranch = _then;
	elseBranch = _else;
	condition = _condition;
	type = ASTType::IF_STMT;
	gc.addASTNode(this);
}

void ASTIfStmt::accept(visitor* vis) {
	vis->visitIfStmt(this);
}


#pragma endregion

#pragma region While stmt
ASTWhileStmt::ASTWhileStmt(ASTNode* _body, ASTNode* _condition) {
	body = _body;
	condition = _condition;
	type = ASTType::WHILE_STMT;
	gc.addASTNode(this);
}

void ASTWhileStmt::accept(visitor* vis) {
	vis->visitWhileStmt(this);
}
#pragma endregion

#pragma region For stmt
ASTForStmt::ASTForStmt(ASTNode* _init, ASTNode* _condition, ASTNode* _increment, ASTNode* _body) {
	init = _init;
	condition = _condition;
	increment = _increment;
	body = _body;
	type = ASTType::FOR_STMT;
	gc.addASTNode(this);
}

void ASTForStmt::accept(visitor* vis) {
	vis->visitForStmt(this);
}
#pragma endregion

#pragma region Foreach stmt
ASTForeachStmt::ASTForeachStmt(Token _varName, ASTNode* _collection, ASTNode* _body) {
	varName = _varName;
	collection = _collection;
	body = _body;
	type = ASTType::FOREACH_STMT;
	gc.addASTNode(this);
}

void ASTForeachStmt::accept(visitor* vis) {
	vis->visitForeachStmt(this);
}
#pragma endregion

#pragma region Break stmt
ASTBreakStmt::ASTBreakStmt(Token _token) {
	token = _token;
	type = ASTType::BREAK_STMT;
	gc.addASTNode(this);
}
void ASTBreakStmt::accept(visitor* vis) {
	vis->visitBreakStmt(this);
}
#pragma endregion

#pragma region Switch stmt
ASTSwitchStmt::ASTSwitchStmt(ASTNode* _expr, vector<ASTNode*>& _cases, switchType _type, bool _hasDefault) {
	expr = _expr;
	cases = _cases;
	type = ASTType::SWITCH_STMT;
	casesType = _type;
	numCases = cases.size();
	hasDefault = _hasDefault;
	gc.addASTNode(this);
}

void ASTSwitchStmt::accept(visitor* vis) {
	vis->visitSwitchStmt(this);
}
#pragma endregion

#pragma region Switch case
ASTCase::ASTCase(ASTNode* _expr, vector<ASTNode*>& _stmts, bool _isDefault) {
	if (_expr != NULL) {
		expr = _expr;
		caseType = ((ASTLiteralExpr*)expr)->getToken().type;
	}
	else {
		expr = NULL;
		caseType = TokenType::TOKEN_ERROR;
	}
	stmts = _stmts;
	type = ASTType::CASE;
	isDefault = _isDefault;
	gc.addASTNode(this);
}

void ASTCase::accept(visitor* vis) {
	vis->visitCase(this);
}
#pragma endregion

#pragma region Return
ASTReturn::ASTReturn(ASTNode* _expr, Token _keyword) {
	expr = _expr;
	keyword = _keyword;
	type = ASTType::RETURN;
	gc.addASTNode(this);
}

void ASTReturn::accept(visitor* vis) {
	vis->visitReturnStmt(this);
}
#pragma endregion




