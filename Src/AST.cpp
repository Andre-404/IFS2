#include "AST.h"
#include "common.h"

/*
accept methods invoke the specific visitor functions for each node type
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


#pragma region Var declaration
ASTVarDecl::ASTVarDecl(Token _name, ASTNode* _setExpr) {
	name = _name;
	setExpr = _setExpr;
	ASTType::VAR_DECL;
}

ASTVarDecl::~ASTVarDecl() {
	if(setExpr != NULL) delete setExpr;
}

void ASTVarDecl::accept(visitor* vis) {
	vis->visitVarDecl(this);
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




