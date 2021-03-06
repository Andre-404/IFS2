#pragma once
#include "common.h"
#include "AST.h"
#include "chunk.h"
#include <string_view>

#ifdef DEBUG_PRINT_AST

class debugASTPrinter : public visitor {
public:
	debugASTPrinter(vector<ASTNode*> _stmts);

	void visitAssignmentExpr(ASTAssignmentExpr* expr);
	void visitSetExpr(ASTSetExpr* expr);
	void visitBinaryExpr(ASTBinaryExpr* expr);
	void visitUnaryExpr(ASTUnaryExpr* expr);
	void visitArrayDeclExpr(ASTArrayDeclExpr* expr);
	void visitCallExpr(ASTCallExpr* expr);
	void visitGroupingExpr(ASTGroupingExpr* expr);
	void visitLiteralExpr(ASTLiteralExpr* expr);

	void visitVarDecl(ASTVarDecl* stmt);
	void visitFuncDecl(ASTFunc* decl);

	void visitPrintStmt(ASTPrintStmt* stmt);
	void visitExprStmt(ASTExprStmt* stmt);
	void visitBlockStmt(ASTBlockStmt* stmt);
	void visitIfStmt(ASTIfStmt* stmt);
	void visitWhileStmt(ASTWhileStmt* stmt);
	void visitForStmt(ASTForStmt* stmt);
	void visitBreakStmt(ASTBreakStmt* stmt);
	void visitSwitchStmt(ASTSwitchStmt* stmt);
	void visitCase(ASTCase* _case);
	void visitReturnStmt(ASTReturn* stmt);
	

private:
	string str;
	bool inLocal;
	void buildExpr(std::string_view name, const std::initializer_list<ASTNode*>& exprs);
};

#endif // DEBUG_PRINT_AST

#pragma region Debug
int disassembleInstruction(chunk* Chunk, int offset);
//static int constantInstruction(string name, chunk* Chunk, int offset);
//static int simpleInstruction(string name, int offset);
//void printValue(Value value);
#pragma endregion

