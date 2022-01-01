#ifndef __IFS_COMPILER
#define __IFS_COMPILER

#include "common.h"
#include "parser.h"
#include "chunk.h"
#include "object.h"

#define LOCAL_MAX 256

struct local {
	Token name;
	int depth;
};

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
	//locals
	local locals[LOCAL_MAX];
	int localCount;
	int scopeDepth;
	//helpers
	void emitByte(uint8_t byte);
	void emitBytes(uint8_t byte1, uint8_t byte2);
	void emitConstant(Value value);
	uint8_t makeConstant(Value value);
	//variables
	uint8_t identifierConstant(Token name);
	void defineVar(uint8_t name);
	void namedVar(Token token, bool canAssign);
	uint8_t parseVar(Token name);
	//locals
	void declareVar(Token& name);
	void addLocal(Token name);
	int resolveLocal(Token& name);
	void markInit();
	void beginScope() { scopeDepth++; }
	void endScope();

	obj* appendObject(obj* _object);
	void emitReturn();
	

	//Visitor pattern
	void visitAssignmentExpr(ASTAssignmentExpr* expr);
	void visitBinaryExpr(ASTBinaryExpr* expr);
	void visitGroupingExpr(ASTGroupingExpr* expr);
	void visitLiteralExpr(ASTLiteralExpr* expr);
	void visitUnaryExpr(ASTUnaryExpr* expr);

	void visitVarDecl(ASTVarDecl* stmt);

	void visitPrintStmt(ASTPrintStmt* stmt);
	void visitExprStmt(ASTExprStmt* stmt);
	void visitBlockStmt(ASTBlockStmt* stmt);
};


#endif // !__IFS_COMPILER

