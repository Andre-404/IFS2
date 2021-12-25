#ifndef __IFS_SCANNER
	#define __IFS_SCANNER
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
		TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE, TOKEN_HASHMAP, TOKEN_SWITCH, TOKEN_FOREACH,

		TOKEN_ERROR, TOKEN_EOF
	};

	struct Token {
		TokenType type;
		string lexeme;
		int line;
	};

	class scanner {
	public:
		scanner(string* src);
		vector<Token>* getArr();
	private:
		string* source;
		int line;
		int start;
		int current;
		vector<Token> arr;

		bool isAtEnd();
		bool match(char expected);
		char advance();
		Token scanToken();
		Token makeToken(TokenType type);
		Token errorToken(const char* message);
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

	};
#endif

