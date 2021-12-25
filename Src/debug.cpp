#include "debug.h"

debugASTPrinter::debugASTPrinter(ASTNode* expr) {
	str = "";
	expr->accept(this);
	std::cout << str;
}

void debugASTPrinter::buildExpr(string name, const std::initializer_list<ASTNode*> &exprs) {
	str.append("(").append(name);
	for (ASTNode* expr : exprs) {
		str.append(" ");
		expr->accept(this);
	}
	str.append(")");
}

void debugASTPrinter::visitBinaryExpr(ASTBinaryExpr* expr) {
	buildExpr(expr->getToken().lexeme, { expr->getLeft(), expr->getRight() });
}
void debugASTPrinter::visitUnaryExpr(ASTUnaryExpr* expr) {
	buildExpr(expr->getToken().lexeme, { expr->getRight() });
}
void debugASTPrinter::visitGroupingExpr(ASTGroupingExpr* expr) {
	buildExpr("group", { expr->getExpr() });
}
void debugASTPrinter::visitLiteralExpr(ASTLiteralExpr* expr) {
	str.append(expr->getToken().lexeme);
}