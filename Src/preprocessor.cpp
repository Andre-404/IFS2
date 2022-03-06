#include "preprocessor.h"
#include "files.h"



preprocessor::preprocessor(string path, string mainFile) {
	filepath = path;
	hadError = false;
	curUnit = nullptr;

	reset();
	preprocessUnit* mainUnit = scanFile(mainFile);
	if (hadError) return;
	topsort(mainUnit);
	expandMacros();
}

preprocessor::~preprocessor() {
	for (preprocessUnit* pUnit : sortedUnits) delete pUnit;
}

void preprocessor::reset() {
	unitsToParse.clear();
}

preprocessUnit* preprocessor::scanFile(string unitName) {
	reset();
	string fullPath = filepath + unitName + ".txt";

	scanner sc(readFile(fullPath));
	preprocessUnit* unit = new preprocessUnit(unitName, sc.getArr(), sc.getFile());
	curUnit = unit;
	allUnits[unitName] = unit;
	//detects #macro and import, and gets rid of newline tokens
	processDirectives(unit);

	vector<Token> depsToParse = unitsToParse;
	for (Token dep : depsToParse) {
		//since import names are strings, we get rid of ""
		string depName = dep.getLexeme();
		depName.erase(0, 1);
		depName.erase(depName.size() - 1, depName.size());
		//if we detect a cyclical import we still continue parsing other files to detect as many errors as possible
		if (allUnits.count(depName) > 0) {
			if (!allUnits[depName]->scanned) {
				error(dep, "Cyclical importing detected.");
				continue;
			}
			unit->deps.push_back(allUnits[depName]);
			continue;
		}
		unit->deps.push_back(scanFile(depName));
	}
	unit->scanned = true;
	return unit;
}

void preprocessor::topsort(preprocessUnit* unit) {
	//TODO: we can get a stack overflow if this goes into deep recursion, try making a iterative stack based DFS implementation
	unit->traversed = true;
	for (preprocessUnit* dep : unit->deps) {
		if (!dep->traversed) topsort(dep);
	}
	sortedUnits.push_back(unit);
}


TokenType checkNext(vector<Token>& tokens, int pos) {
	if (pos + 1 >= tokens.size()) return TOKEN_EOF;
	return tokens[pos + 1].type;
}

bool isAtEnd(vector<Token>& tokens, int pos) {
	return checkNext(tokens, pos) == TOKEN_EOF;
}

bool containsMacro(vector<macro>& macros, macro& _macro) {
	for (int i = 0; i < macros.size(); i++) {
		if (macros[i].name.getLexeme() == _macro.name.getLexeme()) return true;
	}
	return false;
}

vector<Token> replaceFuncMacroParams(macro& toExpand, vector<vector<Token>>& args) {
	//given a set of args, it replaces all params in the macro body with the args
	//this relies on args and params being in same positions in the array
	vector<Token> fin = toExpand.value;
	for (int i = 0; i < fin.size(); i++) {
		Token& token = fin[i];
		if (token.type == TOKEN_IDENTIFIER) {
			for (int j = 0; j < toExpand.params.size(); j++) {
				if (token.getLexeme() == toExpand.params[j].getLexeme()) {
					fin.erase(fin.begin() + i);
					fin.insert(fin.begin() + i, args[j].begin(), args[j].end());
					i--;
				}
			}
		}
	}
	return fin;
}


int parseMacroFuncArgs(vector<vector<Token>>& args, vector<Token>& tokens, int pos) {
	int argCount = 0;
	//very scuffed solution, but it works for when a function call is passed as a argument
	int parenCount = 0;
	args.push_back(vector<Token>());
	while (!isAtEnd(tokens, pos) && checkNext(tokens, pos) != TOKEN_RIGHT_PAREN) {
		//since arguments can be a undefined number of tokens long, we need to parse until we hit either ',' or ')'
		while (!isAtEnd(tokens, pos) && (parenCount > 0 || (checkNext(tokens, pos) != TOKEN_COMMA && checkNext(tokens, pos) != TOKEN_RIGHT_PAREN))) {
			//if parenCount > 0 then we're inside either a call or something else, so we should ignore commas and ')' until parenCount reaches 0
			if (checkNext(tokens, pos) == TOKEN_LEFT_PAREN) parenCount++;
			else if (checkNext(tokens, pos) == TOKEN_RIGHT_PAREN) parenCount--;
			args[argCount].push_back(tokens[++pos]);
		}
		if (checkNext(tokens, pos) == TOKEN_COMMA) pos++;
		argCount++;
		args.push_back(vector<Token>());
	}
	return pos;
}

void preprocessor::expandMacros() {
	for (preprocessUnit* unit : sortedUnits) {
		for (macro& _macro : unit->localMacros) {
			if (macros.count(_macro.name.getLexeme()) > 0) {
				error(_macro.name, "Macro redefinition not allowed.");
				continue;
			}
			macros[_macro.name.getLexeme()] = _macro;
		}
		replaceMacros(unit);
	}
}

vector<Token> preprocessor::expandMacro(macro& toExpand) {
	vector<Token> tokens = toExpand.value;
	for (int i = 0; i < tokens.size(); i++) {
		Token& token = tokens[i];
		if (token.type != TOKEN_IDENTIFIER) continue;
		if (macros.count(token.getLexeme()) == 0) continue;

		//if the macro is already on the stack, it's a recursion
		macro& _macro = macros[token.getLexeme()];
		if (containsMacro(macroStack, _macro) > 0) {
			error(_macro.name, "Recursive macro expansion detected.");
			tokens.clear();
			break;
		}
		macroStack.push_back(_macro);

		if (_macro.isFunctionLike) {
			//get rid of macro name
			tokens.erase(tokens.begin() + i);
			//recursive expansion
			//i - 1 because when we delete the name(if the macro is called correctly) we'll be right at the '(', and since we're
			//always checking ahead, and not at the current position, we need to backtrack a little
			vector<Token> expanded = expandMacro(_macro, tokens, i - 1);
			tokens.insert(tokens.begin() + i, expanded.begin(), expanded.end());
			i--;
			macroStack.pop_back();
			continue;
		}
		//recursivly expands the macro
		vector<Token> expanded = expandMacro(_macro);
		//delete the name
		tokens.erase(tokens.begin() + i);
		tokens.insert(tokens.begin() + i, expanded.begin(), expanded.end());
		i--;
		//pops the macro used for his expansion(to preserve the stack order)
		macroStack.pop_back();
	}
	return tokens;
}

vector<Token> preprocessor::expandMacro(macro& toExpand, vector<Token>& callTokens, int pos) {
	//replaces the params inside the macro body with the args provided
	if (checkNext(callTokens, pos) != TOKEN_LEFT_PAREN) {
		error(callTokens[pos], "Expected a '(' for macro args.");
		return vector<Token>();
	}
	pos++;
	//parses the args
	vector<vector<Token>> args;
	int newPos = parseMacroFuncArgs(args, callTokens, pos);
	if (checkNext(callTokens, newPos) != TOKEN_RIGHT_PAREN) {
		error(callTokens[newPos], "Expected a ')' after macro args.");
		return vector<Token>();
	}
	newPos++;
	//.... macro(arg1, arg2)
	//erases:   ^^^^^^^^^^^^
	callTokens.erase(callTokens.begin() + pos, callTokens.begin() + newPos + 1);
	//this part replaces the params with args
	vector<Token> tokens = replaceFuncMacroParams(toExpand, args);

	for (int i = 0; i < tokens.size(); i++) {
		Token& token = tokens[i];
		if (token.type != TOKEN_IDENTIFIER) continue;
		if (macros.count(token.getLexeme()) == 0) continue;

		macro& _macro = macros[token.getLexeme()];
		if (containsMacro(macroStack, _macro) > 0) {
			error(_macro.name, "Recursive macro expansion detected.");
			tokens.clear();
			break;
		}
		macroStack.push_back(_macro);

		if (_macro.isFunctionLike) {
			//erases the name
			tokens.erase(tokens.begin() + i);
			//i - 1 because when we delete the name(if the macro is called correctly) we'll be right at the '(', and since we're
			//always checking ahead, and not at the current position, we need to backtrack a little
			vector<Token> expanded = expandMacro(_macro, tokens, i - 1);
			tokens.insert(tokens.begin() + i, expanded.begin(), expanded.end());
			i--;
			//preserves stack behaviour
			macroStack.pop_back();
			continue;
		}
		//recursive expansion
		vector<Token> expanded = expandMacro(_macro);
		//erases the name
		tokens.erase(tokens.begin() + i);
		tokens.insert(tokens.begin() + i, expanded.begin(), expanded.end());
		i--;
		//preserves stack behaviour
		macroStack.pop_back();
	}
	return tokens;
}

void preprocessor::replaceMacros(preprocessUnit* unit) {
	auto it = unit->tokens.begin();
	for (int i = 0; i < unit->tokens.size(); i++) {
		Token& token = unit->tokens[i];
		if (token.type != TOKEN_IDENTIFIER) continue;
		if (macros.count(token.getLexeme()) == 0) continue;
		macro& _macro = macros[token.getLexeme()];
		
		//if a function is macro like we need to parse it's arguments and replace the params inside the macro body with the provided args
		//this is a simple token replacement
		if (_macro.isFunctionLike) {
			//delete macro name
			unit->tokens.erase(it + i);
			it = unit->tokens.begin();
			//calling a function here to detect possible cyclical macros
			macroStack.push_back(_macro);
			vector<Token> expanded = expandMacro(_macro, unit->tokens, i - 1);
			macroStack.pop_back();
			//for better error detection
			for (Token expandedToken : expanded) expandedToken.line = token.line;
			//insert the macro body with the params replaced by args
			it = unit->tokens.begin();
			unit->tokens.insert(it + i, expanded.begin(), expanded.end());
			i--;
			continue;
		}
		macroStack.push_back(_macro);
		vector<Token> expanded = expandMacro(_macro);
		macroStack.pop_back();
		//get rid of macro name
		unit->tokens.erase(it + i);
		unit->tokens.insert(it + i, expanded.begin(), expanded.end());
		i--;
		it = unit->tokens.begin();
	}
}

void preprocessor::processDirectives(preprocessUnit* unit) {
	vector<Token>& tokens = unit->tokens;
	//Doing tokens.begin() instead of declaring a it here is dumb, but it needs to update every time we delete/insert stuff
	for (int i = 0; i < tokens.size(); i++) {
		Token& token = tokens[i];
		if (token.type == TOKEN_MACRO) {
			if (checkNext(tokens, i) != TOKEN_IDENTIFIER) {
				error(tokens[i], "Expected macro name");
				break;
			}
			//deletes the #macro and macro name
			tokens.erase(tokens.begin() + i);
			Token name = tokens[i];
			tokens.erase(tokens.begin() + i--);

			macro curMacro = macro(name);
			//detecting if we have a function like macro(eg. #macro add(a, b): (a + b)
			if (checkNext(tokens, i) == TOKEN_LEFT_PAREN) {
				int curPos = ++i;
				//parses arguments
				while (!isAtEnd(tokens, i) && checkNext(tokens, i) != TOKEN_RIGHT_PAREN) {
					Token& arg = tokens[++i];
					//checking if we already have a param of the same name
					for (int j = 0; j < curMacro.params.size(); j++) {
						if (arg.getLexeme() == curMacro.params[j].getLexeme()) {
							error(arg, "Cannot have 2 or more arguments of the same name.");
							break;
						}
					}
					curMacro.params.push_back(arg);
					if (checkNext(tokens, i) == TOKEN_COMMA) ++i;
				}
				//we break here because if we're at EOF and try erasing i + 1 we'll get a error
				if (isAtEnd(tokens, i) || checkNext(tokens, i) != TOKEN_RIGHT_PAREN) {
					error(tokens[i], "Expected ')' after arguments.");
					break;
				}
				i++;
				//#macro add(a, b): (a + b)
				//deletes   ^^^^^^
				auto it = tokens.begin();
				tokens.erase(it + curPos, it + i + 1);
				i = curPos - 1;
				curMacro.isFunctionLike = true;
			}
			//every macro declaration must have ':' before it's body(this is used to differentiate between normal macros and function like macros
			if (checkNext(tokens, i) != TOKEN_COLON) {
				error(tokens[i], "Expected ':' following macro name.");
				break;
			}
			tokens.erase(tokens.begin() + i + 1);
			//loops until we hit a \n
			//for each iteration, we add the token to macro body and then delete it(keeping i in check so that we don't go outside of boundries)
			while (!isAtEnd(tokens, i) && checkNext(tokens, i) != TOKEN_NEWLINE) {
				curMacro.value.push_back(tokens[++i]);
				tokens.erase(tokens.begin() + i--);
			}
			unit->localMacros.push_back(curMacro);
		}
		else if (token.type == TOKEN_IMPORT) {
			if (checkNext(tokens, i) != TOKEN_STRING) error(tokens[i], "Expected a module name.");
			//deletes both the import and module name tokens from the token list, and add them to toParse
			tokens.erase(tokens.begin() + i);
			Token name = tokens[i];
			tokens.erase(tokens.begin() + i--);
			unitsToParse.push_back(name);
		}
		else if (token.type == TOKEN_NEWLINE) {
			//this token is used for macro bodies, and isn't needed for anything else, so we delete it when we find it
			tokens.erase(tokens.begin() + i--);
		}
	}
}

void preprocessor::error(Token token, string msg) {
	hadError = true;
	std::cout << "Error " << "[line " << token.line << "]" << " in '" << curUnit->name << "':" << "\n";
	report(curUnit->srcFile, token, msg);
}