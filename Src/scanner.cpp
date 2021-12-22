#include "scanner.h"



scanner::scanner(string* src) {
	line = 1;
	vector<Token> arr;
	source = src;
	start = 0;
	current = start;

	Token token = scanToken();
	arr.push_back(token);
	while (token.type != TOKEN_EOF) {
		token = scanToken();
		arr.push_back(token);
	}
	arr.push_back(token);
	std::cout << arr.size()<<"\n";
	for (int i = 0; i < arr.size(); i++) {
		Token tok = arr[i];
		std::cout << tok.lexeme<<"\n";
	}
}

	bool scanner::isAtEnd() {
		return current >= source->size();
	}

	bool scanner::match(char expected) {
		if (isAtEnd()) return false;
		if ((*source).at(current) != expected) return false;
		current++;
		return true;
	}

	char scanner::advance() {
		return (*source).at(current++);
	}

	Token scanner::scanToken() {
		skipWhitespace();
		start = current;

		if (isAtEnd()) return makeToken(TOKEN_EOF);

		char c = advance();
		if (isDigit(c)) return number();
		if (isAlpha(c)) return identifier();

		switch (c) {
			case '(': return makeToken(TOKEN_LEFT_PAREN);
			case ')': return makeToken(TOKEN_RIGHT_PAREN);
			case '{': return makeToken(TOKEN_LEFT_BRACE);
			case '}': return makeToken(TOKEN_RIGHT_BRACE);
			case '[': return makeToken(TOKEN_LEFT_BRACKET);
			case ']': return makeToken(TOKEN_RIGHT_BRACKET);
			case ';': return makeToken(TOKEN_SEMICOLON);
			case ',': return makeToken(TOKEN_COMMA);
			case '.': return makeToken(TOKEN_DOT);
			case '-': return makeToken(TOKEN_MINUS);
			case '+': return makeToken(TOKEN_PLUS);
			case '/': return makeToken(TOKEN_SLASH);
			case '*': return makeToken(TOKEN_STAR);
			case '!':
				return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
			case '=':
				return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
			case '<':
				return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
			case '>':
				return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
			case '"': return string_();
		}

		return errorToken("Unexpected character.");
	}

	Token scanner::makeToken(TokenType type) {
		Token token;
		token.type = type;
		string buffer;
		for (int i = start; i < current; i++) {
			buffer += source->at(i);
		}
		token.lexeme = buffer;
		token.line = line;
		return token;
	}

	Token scanner::errorToken(const char* message) {
		Token token;
		token.type = TOKEN_ERROR;
		token.lexeme = string(message);
		token.line = line;
		return token;
	}
	char scanner::peek() {
		if (isAtEnd()) return '\0';
		return source->at(current);
	}
	char scanner::peekNext() {
		if (isAtEnd()) return '\0';
		return source->at(current+1);
	}
	void scanner::skipWhitespace() {
		for (;;) {
			char c = peek();
			switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
				break;
			case '\n':
				line++;
				advance();
				break;
			case '/':
				if (peekNext() == '/') {
					// A comment goes until the end of the line.
					while (peek() != '\n' && !isAtEnd()) advance();
				}
				else {
					return;
				}
				break;
			default:
				return;
			}
		}
	}
	Token scanner::string_() {
		while (peek() != '"' && !isAtEnd()) {
			if (peek() == '\n') line++;
			advance();
		}

		if (isAtEnd()) return errorToken("Unterminated string.");

		// The closing quote.
		advance();
		return makeToken(TOKEN_STRING);
	}

	bool scanner::isDigit(char c) {
		return c >= '0' && c <= '9';
	}

	Token scanner::number() {
		while (isDigit(peek())) advance();

		// Look for a fractional part.
		if (peek() == '.' && isDigit(peekNext())) {
			// Consume the ".".
			advance();

			while (isDigit(peek())) advance();
		}

		return makeToken(TOKEN_NUMBER);
	}

	bool scanner::isAlpha(char c) {
		return (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			c == '_';
	}

	Token scanner::identifier() {
		while (isAlpha(peek()) || isDigit(peek())) advance();
		return makeToken(identifierType());
	}

	TokenType scanner::identifierType() {
		switch (source->at(start)) {
			case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
			case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
			case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
			case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
			case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
			case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
			case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
			case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
			case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
			case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
			case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
			case 'f':
				if (current - start > 1) {
					switch (source->at(start+1)) {
					case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
					case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
					case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
					}
				}
				break;
			case 't':
				if (current - start > 1) {
					switch (source->at(start)) {
					case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
					case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
					}
				}
				break;
		}
	}

	TokenType scanner::checkKeyword(int start, int length, const char* rest, TokenType type) {
		if (current - start == start + length && source->compare(rest) == 0) {
			return type;
		}

		return TOKEN_IDENTIFIER;
	}