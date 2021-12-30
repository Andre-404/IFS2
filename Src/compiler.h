#ifndef __IFS_COMPILER
#define __IFS_COMPILER

#include "common.h"
#include "parser.h"
#include "chunk.h"
#include "object.h"

class compiler : public visitor {
public:
	bool compiled;
	obj* objects;//linked list
	compiler(parser* Parser);
	~compiler();
	chunk* getCurrent();
private:
	chunk* current;
	int line;
	void emitByte(uint8_t byte);
	void emitBytes(uint8_t byte1, uint8_t byte2);
	void emitConstant(Value value);
	uint8_t makeConstant(Value value);
	uint8_t identifierConstant(Token name);
	void defineVar(uint8_t name);
	void namedVar(Token token);

	obj* appendObject(obj* _object);
	void emitReturn();

	void visitAssignmentExpr(ASTAssignmentExpr* expr);
	void visitBinaryExpr(ASTBinaryExpr* expr);
	void visitGroupingExpr(ASTGroupingExpr* expr);
	void visitLiteralExpr(ASTLiteralExpr* expr);
	void visitUnaryExpr(ASTUnaryExpr* expr);

	void visitVarDecl(ASTVarDecl* stmt);

	void visitPrintStmt(ASTPrintStmt* stmt);
	void visitExprStmt(ASTExprStmt* stmt);
};


#endif // !__IFS_COMPILER

