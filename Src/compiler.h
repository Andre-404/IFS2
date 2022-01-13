#ifndef __IFS_COMPILER
#define __IFS_COMPILER

#include "common.h"
#include "parser.h"
#include "chunk.h"
#include "object.h"

#define LOCAL_MAX 256

enum class funcType {
	TYPE_FUNC,
	TYPE_SCRIPT
};

struct local {
	string name;
	int depth = -1;
};

struct _break {
	int depth = 0;
	int offset = -1;
	int varNum = 0;
	_break(int _d, int _o, int _varNum) : depth(_d), offset(_o), varNum(_varNum) {}
};

struct compilerInfo {
	//for closures
	compilerInfo* enclosing;
	//function that's currently being compiled
	objFunc* func = new objFunc();
	chunk* code = &func->body;
	funcType type;

	int line = 0;
	//keeps track of every break statement that has been encountered
	vector<_break> breakStmts;
	//locals
	local locals[LOCAL_MAX];
	int localCount = 0;
	int scopeDepth = 0;
	compilerInfo(compilerInfo* _enclosing, funcType _type);
};

class compiler : public visitor {
public:
	bool compiled;
	compiler(parser* Parser, funcType _type);//for compiling top level code
	~compiler();
	chunk* getChunk();
	objFunc* endFuncDecl();
private:
	//compiler only ever emits the code for a single function, top level code is considered a function
	compilerInfo* current;

	parser* Parser;

	#pragma region Helpers
	//emitters
	void emitByte(uint8_t byte);
	void emitBytes(uint8_t byte1, uint8_t byte2);
	void emit16Bit(unsigned short number);
	void emitConstant(Value value);
	void emitReturn();
	//control flow
	int emitJump(OpCode jumpType);
	void patchJump(int offset);
	void emitLoop(int start);
	void patchBreak();
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
	void beginScope() { current->scopeDepth++; }
	void endScope();
	
	#pragma endregion
	

	//Visitor pattern
	void visitAssignmentExpr(ASTAssignmentExpr* expr);
	void visitSetExpr(ASTSetExpr* expr);
	void visitBinaryExpr(ASTBinaryExpr* expr);
	void visitGroupingExpr(ASTGroupingExpr* expr);
	void visitArrayDeclExpr(ASTArrayDeclExpr* expr);
	void visitCallExpr(ASTCallExpr* expr);
	void visitLiteralExpr(ASTLiteralExpr* expr);
	void visitUnaryExpr(ASTUnaryExpr* expr);

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
};


#endif // !__IFS_COMPILER

