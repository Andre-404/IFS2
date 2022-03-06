#pragma once
#include "common.h"
using std::vector;

enum TokenType {
	// Single-character tokens.
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR, TOKEN_PERCENTAGE,
	TOKEN_QUESTIONMARK, TOKEN_COLON, TOKEN_TILDA,
	// One or two character tokens.
	TOKEN_BITWISE_AND, TOKEN_BITWISE_OR, TOKEN_BITWISE_XOR,
	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, 
	TOKEN_PERCENTAGE_EQUAL, TOKEN_BITWISE_AND_EQUAL, TOKEN_BITWISE_OR_EQUAL, TOKEN_BITWISE_XOR_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,
	TOKEN_BITSHIFT_LEFT, TOKEN_BITSHIFT_RIGHT, 
	TOKEN_INCREMENT, TOKEN_DECREMENT,
	// Literals.
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
	// Keywords.
	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FOR, TOKEN_FUNC, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE, TOKEN_HASHMAP, 
	TOKEN_SWITCH, TOKEN_FOREACH, TOKEN_BREAK, TOKEN_CASE, TOKEN_DEFAULT,
	TOKEN_IMPORT, TOKEN_MACRO, TOKEN_YIELD, TOKEN_FIBER, TOKEN_RUN,

	TOKEN_NEWLINE, TOKEN_ERROR, TOKEN_EOF
};


struct file {
	string sourceFile;
	std::vector<uInt> lines;
	file(string& src) : sourceFile(src) {};
};

struct span {
	uInt64 line;
	uInt64 column;
	uInt64 length;
	file* sourceFile;
	span() {
		line = 0;
		column = 0;
		length = 0;
		sourceFile = nullptr;
	}
	span(uInt64 _line, uInt64 _column, uInt64 _len, file* _src) : line(_line), column(_column), length(_len), sourceFile(_src) {};
	string getStr();
};

struct Token {
	TokenType type;
	span str;
	//for things like synthetic tokens and expanded macros
	long long line;
	
	bool isSynthetic;
	const char* ptr;
	
	Token() {
		isSynthetic = false;
		ptr = nullptr;
		line = -1;
		type = TOKEN_LEFT_PAREN;
	}
	Token(span _str, TokenType _type) {
		isSynthetic = false;
		line = _str.line;
		ptr = nullptr;
		str = _str;
		type = _type;
	}
	Token(const char* _ptr, uInt64 _line, TokenType _type) {
		ptr = _ptr;
		isSynthetic = true;
		type = _type;
		line = _line;
	}
	string getLexeme() {
		if (isSynthetic) return string(ptr);
		return str.getStr();
	}
};


class scanner {
public:
	scanner(string source);
	vector<Token> getArr();
	file* getFile() { return curFile; }
private:
	file* curFile;
	int line;
	int start;
	int current;
	bool hadError;
	vector<Token> tokens;

	Token scanToken();
	Token makeToken(TokenType type);
	Token errorToken(const char* message);

	bool isAtEnd();
	bool match(char expected);
	char advance();
	char peek();
	char peekNext();
	void skipWhitespace();

	Token string_();

	bool isDigit(char c);
	Token number();

	bool isAlpha(char c);
	Token identifier();
	TokenType identifierType();
	TokenType checkKeyword(int start, int length, const char* rest, TokenType type);

	bool isPreprocessDirective(char c);
	TokenType directiveType();
};

void report(file* src, Token& token, string msg);

