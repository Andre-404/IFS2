#ifndef __IFS_DEBUG
#define __IFS_DEBUG
#include "common.h"
#include "AST.h"

class debugASTPrinter : public visitor {
public:
	debugASTPrinter(ASTNode* expr);
	void visitBinaryExpr(ASTBinaryExpr* expr);
	void visitUnaryExpr(ASTUnaryExpr* expr);
	void visitGroupingExpr(ASTGroupingExpr* expr);
	void visitLiteralExpr(ASTLiteralExpr* expr);

private:
	string str;
	void buildExpr(string name, const std::initializer_list<ASTNode*>& exprs);
};

#endif // !__IFS_DEBUG

