#include "AST.h"
#include "common.h"

/*
accept methods invoke the specific visitor functions for each node type
*/

#pragma region Binary
ASTBinaryExpr::ASTBinaryExpr(ASTNode* _left, Token _op, ASTNode* _right) {
	op = _op;
	left = _left;
	right = _right;
}

ASTBinaryExpr::~ASTBinaryExpr() {
	delete left;
	delete right;
}

void ASTBinaryExpr::accept(visitor* vis) {
	vis->visitBinaryExpr(this);
}
#pragma endregion

#pragma region Unary
ASTUnaryExpr::ASTUnaryExpr(Token _op, ASTNode* _right) {
	op = _op;
	right = _right;
}

ASTUnaryExpr::~ASTUnaryExpr() {
	delete right;
}

void ASTUnaryExpr::accept(visitor* vis) {
	vis->visitUnaryExpr(this);
}
#pragma endregion

#pragma region Grouping
ASTGroupingExpr::ASTGroupingExpr(ASTNode* _expr) {
	expr = _expr;
}

ASTGroupingExpr::~ASTGroupingExpr() {
	delete expr;
}

void ASTGroupingExpr::accept(visitor* vis) {
	vis->visitGroupingExpr(this);
}
#pragma endregion

#pragma region Literal
ASTLiteralExpr::ASTLiteralExpr(Token _token) {
	token = _token;
}

void ASTLiteralExpr::accept(visitor* vis) {
	vis->visitLiteralExpr(this);
}
#pragma endregion
