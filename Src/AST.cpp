#include "AST.h"
#include "common.h"

/*
accept methods invoke the specific visitor functions for each node type
WARNING: don't forget to add 'type' field in the constructor
*/
#pragma region Assigment expr
ASTAssignmentExpr::ASTAssignmentExpr(Token _name, ASTNode* _value) {
	name = _name;
	value = _value;
	type = ASTType::ASSINGMENT;
}

ASTAssignmentExpr::~ASTAssignmentExpr() {
	delete value;
}

void ASTAssignmentExpr::accept(visitor* vis) {
	vis->visitAssignmentExpr(this);
}

#pragma endregion

#pragma region Or expr
ASTOrExpr::ASTOrExpr(ASTNode* _left, ASTNode* _right) {
	left = _left;
	right = _right;
	type = ASTType::OR;
}
ASTOrExpr::~ASTOrExpr() {
	delete left;
	delete right;
}
void ASTOrExpr::accept(visitor* vis) {
	vis->visitOrExpr(this);
}

#pragma endregion

#pragma region And expr
ASTAndExpr::ASTAndExpr(ASTNode* _left, ASTNode* _right) {
	left = _left;
	right = _right;
	type = ASTType::AND;
}
ASTAndExpr::~ASTAndExpr() {
	delete left;
	delete right;
}

void ASTAndExpr::accept(visitor* vis) {
	vis->visitAndExpr(this);
}
#pragma endregion

#pragma region Binary expr
ASTBinaryExpr::ASTBinaryExpr(ASTNode* _left, Token _op, ASTNode* _right) {
	op = _op;
	left = _left;
	right = _right;
	type = ASTType::BINARY;
}

ASTBinaryExpr::~ASTBinaryExpr() {
	delete left;
	delete right;
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
}

ASTUnaryExpr::~ASTUnaryExpr() {
	delete right;
}

void ASTUnaryExpr::accept(visitor* vis) {
	vis->visitUnaryExpr(this);
}
#pragma endregion

#pragma region Call expr
ASTCallExpr::ASTCallExpr(ASTNode* _callee, vector<ASTNode*>& _args) {
	callee = _callee;
	args = _args;
	type = ASTType::CALL;
}
ASTCallExpr::~ASTCallExpr() {
	delete callee;
	for (ASTNode* node : args) {
		delete node;
	}
}

void ASTCallExpr::accept(visitor* vis) {
	vis->visitCallExpr(this);
}

#pragma endregion

#pragma region Grouping expr
ASTGroupingExpr::ASTGroupingExpr(ASTNode* _expr) {
	expr = _expr;
	ASTType::GROUPING;
}

ASTGroupingExpr::~ASTGroupingExpr() {
	delete expr;
}

void ASTGroupingExpr::accept(visitor* vis) {
	vis->visitGroupingExpr(this);
}
#pragma endregion

#pragma region Literal expr
ASTLiteralExpr::ASTLiteralExpr(Token _token) {
	token = _token;
	type = ASTType::LITERAL;
}

void ASTLiteralExpr::accept(visitor* vis) {
	vis->visitLiteralExpr(this);
}
#pragma endregion


#pragma region Var decl
ASTVarDecl::ASTVarDecl(Token _name, ASTNode* _setExpr) {
	name = _name;
	setExpr = _setExpr;
	type = ASTType::VAR_DECL;
}

ASTVarDecl::~ASTVarDecl() {
	if(setExpr != NULL) delete setExpr;
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
}
ASTFunc::~ASTFunc() {
	delete body;
}

void ASTFunc::accept(visitor* vis) {
	vis->visitFuncDecl(this);
}
#pragma endregion



#pragma region Print stmt
ASTPrintStmt::ASTPrintStmt(ASTNode* _expr) {
	expr = _expr;
	type = ASTType::PRINT_STMT;
}

ASTPrintStmt::~ASTPrintStmt() {
	delete expr;
}

void ASTPrintStmt::accept(visitor* vis) {
	vis->visitPrintStmt(this);
}
#pragma endregion

#pragma region Expr stmt
ASTExprStmt::ASTExprStmt(ASTNode* _expr) {
	expr = _expr;
	ASTType::EXPR_STMT;
}

ASTExprStmt::~ASTExprStmt() {
	delete expr;
}

void ASTExprStmt::accept(visitor* vis) {
	vis->visitExprStmt(this);
}


#pragma endregion

#pragma region Block stmt
ASTBlockStmt::ASTBlockStmt(vector<ASTNode*>& _statements) {
	statements = _statements;
	type = ASTType::BLOCK_STMT;
}

ASTBlockStmt::~ASTBlockStmt() {
	for (ASTNode* node : statements) {
		delete node;
	}
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
}
ASTIfStmt::~ASTIfStmt() {
	delete thenBranch;
	delete elseBranch;
	delete condition;
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
}
ASTWhileStmt::~ASTWhileStmt() {
	delete body;
	delete condition;
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
}

ASTForStmt::~ASTForStmt() {
	delete init;
	delete condition;
	delete increment;
	delete body;
}

void ASTForStmt::accept(visitor* vis) {
	vis->visitForStmt(this);
}
#pragma endregion

#pragma region Break stmt
ASTBreakStmt::ASTBreakStmt(Token _token) {
	token = _token;
	type = ASTType::BREAK_STMT;
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
}
ASTSwitchStmt::~ASTSwitchStmt() {
	delete expr;
	for (ASTNode* _case : cases) {
		delete _case;
	}
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
}
ASTCase::~ASTCase() {
	delete expr;
	for (ASTNode* stmt : stmts) {
		delete stmt;
	}
}
void ASTCase::accept(visitor* vis) {
	vis->visitCase(this);
}
#pragma endregion

#pragma region Return
ASTReturn::ASTReturn(ASTNode* _expr) {
	expr = _expr;
	type = ASTType::RETURN;
}
ASTReturn::~ASTReturn() {
	delete expr;
}

void ASTReturn::accept(visitor* vis) {
	vis->visitReturnStmt(this);
}
#pragma endregion



