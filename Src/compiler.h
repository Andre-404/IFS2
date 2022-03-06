#pragma once

#include "common.h"
#include "parser.h"
#include "chunk.h"
#include "object.h"
#include <array>


#define LOCAL_MAX 512
#define UPVAL_MAX 256

enum class funcType {
	TYPE_FUNC,
	TYPE_METHOD,
	TYPE_CONSTRUCTOR,
	TYPE_SCRIPT,
	TYPE_FIBER
};

struct local {
	string name = "";
	int depth = -1;
	bool isCaptured = false;
};

struct upvalue{
	uint8_t index;
	bool isLocal;
	upvalue() {
		index = 0;
		isLocal = false;
	}
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
	objFunc* func;
	funcType type;
	bool hasReturn;

	int line;
	//keeps track of every break statement that has been encountered
	vector<_break> breakStmts;
	//locals
	local locals[LOCAL_MAX];
	int localCount;
	int scopeDepth;
	std::array<upvalue, UPVAL_MAX> upvalues;
	bool hasCapturedLocals;
	compilerInfo(compilerInfo* _enclosing, funcType _type);
};

struct classCompilerInfo {
	classCompilerInfo* enclosing;
	bool hasSuperclass;
	classCompilerInfo(classCompilerInfo* _enclosing, bool _hasSuperclass) : enclosing(_enclosing), hasSuperclass(_hasSuperclass) {};
};

class compiler : public visitor {
public:
	//compiler only ever emits the code for a single function, top level code is considered a function
	compilerInfo* current;
	classCompilerInfo* currentClass;
	bool compiled;
	compiler(string path, string fileName, funcType _type);//for compiling top level code
	chunk* getChunk();
	objFunc* endFuncDecl();
private:

	#pragma region Helpers
	//emitters
	void emitByte(uint8_t byte);
	void emitBytes(uint8_t byte1, uint8_t byte2);
	void emit16Bit(uInt16 number);
	void emitByteAnd16Bit(uint8_t byte, uInt num);
	void emitConstant(Value value);
	void emitReturn();
	//control flow
	int emitJump(OpCode jumpType);
	void patchJump(int offset);
	void emitLoop(int start);
	void patchBreak();
	uInt makeConstant(Value value);
	//variables
	uInt identifierConstant(Token name);
	void defineVar(uInt name);
	void namedVar(Token token, bool canAssign);
	uInt parseVar(Token name);
	//locals
	void declareVar(Token& name);
	void addLocal(Token name);
	int resolveLocal(Token& name);
	int resolveLocal(compilerInfo* func, Token& name);
	int resolveUpvalue(compilerInfo* func, Token& name);
	int addUpvalue(uint8_t index, bool isLocal);
	void markInit();
	void beginScope() { current->scopeDepth++; }
	void endScope();
	//classes and methods
	void method(ASTFunc* _method, Token className);
	bool invoke(ASTCallExpr* expr);
	Token syntheticToken(const char* str);
	//misc
	void updateLine(Token token);
	#pragma endregion
	

	#pragma region Visitor pattern
	void visitAssignmentExpr(ASTAssignmentExpr* expr);
	void visitSetExpr(ASTSetExpr* expr);
	void visitConditionalExpr(ASTConditionalExpr* expr);
	void visitBinaryExpr(ASTBinaryExpr* expr);
	void visitGroupingExpr(ASTGroupingExpr* expr);
	void visitArrayDeclExpr(ASTArrayDeclExpr* expr);
	void visitUnaryExpr(ASTUnaryExpr* expr);
	void visitCallExpr(ASTCallExpr* expr);
	void visitUnaryVarAlterExpr(ASTUnaryVarAlterExpr* expr);
	void visitStructLiteralExpr(ASTStructLiteral* expr);
	void visitSuperExpr(ASTSuperExpr* expr);
	void visitLiteralExpr(ASTLiteralExpr* expr);
	void visitYieldExpr(ASTYieldExpr* expr);
	void visitFiberLiteralExpr(ASTFiberLiteral* expr);
	void visitFiberRunExpr(ASTFiberRunExpr* expr);
	

	void visitVarDecl(ASTVarDecl* decl);
	void visitFuncDecl(ASTFunc* decl);
	void visitClassDecl(ASTClass* decl);

	void visitPrintStmt(ASTPrintStmt* stmt);
	void visitExprStmt(ASTExprStmt* stmt);
	void visitBlockStmt(ASTBlockStmt* stmt);
	void visitIfStmt(ASTIfStmt* stmt);
	void visitWhileStmt(ASTWhileStmt* stmt);
	void visitForStmt(ASTForStmt* stmt);
	void visitForeachStmt(ASTForeachStmt* stmt);
	void visitBreakStmt(ASTBreakStmt* stmt);
	void visitSwitchStmt(ASTSwitchStmt* stmt);
	void visitCase(ASTCase* _case);
	void visitReturnStmt(ASTReturn* stmt);
	#pragma endregion 
};

vector<translationUnit*> topsortUnits(translationUnit* startingUnit);

