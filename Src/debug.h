#ifndef __IFS_DEBUG
#define __IFS_DEBUG
#include "common.h"
#include "AST.h"
#include "chunk.h"

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

#pragma region Debug
int disassembleInstruction(chunk* Chunk, int offset);
static int constantInstruction(string name, chunk* Chunk, int offset);
static int simpleInstruction(string name, int offset);
void printValue(Value value);
#pragma endregion


#endif // !__IFS_DEBUG

