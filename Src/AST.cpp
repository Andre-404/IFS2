#include "AST.h"
#include "common.h"


#pragma region Binary
ASTBinaryExpr::ASTBinaryExpr(Token _op, ASTNode* _left, ASTNode* _right) {
	op = _op;
	left = _left;
	right = _right;
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

void ASTUnaryExpr::accept(visitor* vis) {
	vis->visitUnaryExpr(this);
}
#pragma endregion

#pragma region Grouping
ASTGroupingExpr::ASTGroupingExpr(ASTNode* _expr) {
	expr = _expr;
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
