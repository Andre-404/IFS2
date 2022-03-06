#pragma once
#include "common.h"
#include "scanner.h"
#include <unordered_map>

struct macro {
	Token name;
	std::vector<Token> value;
	bool isFunctionLike = false;
	std::vector<Token> params;

	macro() {};
	macro(Token _name) : name(_name) {};
};

struct preprocessUnit {
	std::vector<macro> localMacros;
	std::vector<Token> tokens;
	std::vector<preprocessUnit*> deps;

	file* srcFile;

	string name;
	bool traversed;
	bool scanned;
	preprocessUnit(string _name, vector<Token> _tokens, file* _src) : name(_name), traversed(false), scanned(false), tokens(_tokens), srcFile(_src) {};
};

class preprocessor {
public:
	preprocessor(string path, string mainFile);
	~preprocessor();

	vector<preprocessUnit*> getSortedUnits() { return sortedUnits; }

	bool hadError;
private:
	string filepath;
	preprocessUnit* curUnit;
	
	std::unordered_map<string, preprocessUnit*> allUnits;
	std::unordered_map<string, macro> macros;
	vector<macro> macroStack;
	vector<Token> unitsToParse;
	vector<preprocessUnit*> sortedUnits;

	void processDirectives(preprocessUnit* unit);

	void expandMacros();
	vector<Token> expandMacro(macro& _macro);
	vector<Token> expandMacro(macro& _macro, vector<Token>& callTokens, int pos);
	void replaceMacros(preprocessUnit* unit);

	void reset();
	preprocessUnit* scanFile(string unitName);
	void topsort(preprocessUnit* unit);

	void error(Token token, string msg);
};