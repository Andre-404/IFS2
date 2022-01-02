#include "scanner.h"
#include <string_view>

scanner::scanner(string* src) {
	line = 1;
	source = src;
	start = 0;
	current = start;

	Token token = scanToken();
	arr.push_back(token);
	while (token.type != TOKEN_EOF) {
		token = scanToken();
		arr.push_back(token);
	}
}

vector<Token>* scanner::getArr() {
	return &arr;
}

bool scanner::isAtEnd() {
	return current >= source->size();
}

//if matched we consume the token
bool scanner::match(char expected) {
	if (isAtEnd()) return false;
	if (source->at(current) != expected) return false;
	current++;
	return true;
}

char scanner::advance() {
	return source->at(current++);
}

Token scanner::scanToken() {
	skipWhitespace();
	start = current;

	if (isAtEnd()) return makeToken(TOKEN_EOF);

	char c = advance();
	//identifiers start with _ or [a-z][A-Z]
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
		case '-': return makeToken(match('=') ? TOKEN_MINUS_EQUAL : match('-') ? TOKEN_DECREMENT : TOKEN_MINUS);
		case '+': return makeToken(match('=') ? TOKEN_PLUS_EQUAL : match('+') ? TOKEN_INCREMENT : TOKEN_PLUS);
		case '/': return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
		case '*': return makeToken(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
		case '&': return makeToken(match('=') ? TOKEN_BITWISE_AND_EQUAL : match('&') ? TOKEN_AND : TOKEN_BITWISE_AND);
		case '|': return makeToken(match('=') ? TOKEN_BITWISE_OR_EQUAL : match('|') ? TOKEN_OR : TOKEN_BITWISE_OR);
		case '^': return makeToken(match('=') ? TOKEN_BITWISE_XOR_EQUAL : TOKEN_BITWISE_XOR);
		case '%': return makeToken(match('=') ? TOKEN_PERCENTAGE_EQUAL : TOKEN_PERCENTAGE);
		case '~': return makeToken(TOKEN_TILDA);
		case '!':
			return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=':
			return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<':
			return makeToken(match('=') ? TOKEN_LESS_EQUAL : match('<') ? TOKEN_BITSHIFT_LEFT : TOKEN_LESS);
		case '>':
			return makeToken(match('=') ? TOKEN_GREATER_EQUAL : match('>') ? TOKEN_BITSHIFT_RIGHT : TOKEN_GREATER);
		case '"': return string_();
	}

	return errorToken("Unexpected character.");
}
//creates a string_view to save memory, points back to source
Token scanner::makeToken(TokenType type) {
	Token token;
	token.type = type;
	token.lexeme = std::string_view(source->c_str() + start, current-start);
	token.line = line;
	return token;
}

Token scanner::errorToken(const char* message) {
	Token token;
	token.type = TOKEN_ERROR;
	token.lexeme = std::string_view(message);
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
	while(true) {
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

//trie implementation
TokenType scanner::identifierType() {
	switch (source->at(start)) {
		case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
		case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
		case 'c': 
			if (current - start > 1) {
				switch (source->at(start + 1)) {
				case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
				case 'a': return checkKeyword(2, 2, "se", TOKEN_CASE);
				}
			}
			break;
		case 'd': return checkKeyword(1, 6, "efault", TOKEN_DEFAULT);
		case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
		case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
		case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
		case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
		case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
		case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
		case 's': 
			if (current - start > 1) {
				switch (source->at(start + 1)) {
					case 'u': return checkKeyword(1, 3, "per", TOKEN_SUPER);
					case 'w': return checkKeyword(1, 4, "itch", TOKEN_SWITCH);
				}
			}
			break;
		case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
		case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
		case 'f':
			if (current - start > 1) {
				switch (source->at(start+1)) {
				case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
				case 'o': 
					//since foreach has the same letters as for we first check the total number of letters
					if (current - start > 3) {
						return checkKeyword(2, 5, "reach", TOKEN_FOREACH);
					}
					return checkKeyword(2, 1, "r", TOKEN_FOR);
				case 'u': return checkKeyword(2, 2, "nc", TOKEN_FUNC);
				}
			}
			break;
		case 't':
			if (current - start > 1) {
				switch (source->at(start+1)) {
				case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
				case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			break;
	}
	return TOKEN_IDENTIFIER;
}

//is string.compare fast enough?
TokenType scanner::checkKeyword(int strt, int length, const char* rest, TokenType type) {
	if (current - start == strt + length && source->substr(start + strt, length).compare(rest) == 0) {
		return type;
	}

	return TOKEN_IDENTIFIER;
}