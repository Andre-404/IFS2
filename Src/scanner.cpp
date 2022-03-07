#include "scanner.h"
#include "files.h"

string span::getStr() {
	uInt start = sourceFile->lines[line - 1] + column;
	return sourceFile->sourceFile.substr(start, length);
}

scanner::scanner(string _source, string _sourceName) {
	curFile = new file(_source, _sourceName);
	line = 1;
	start = 0;
	current = start;
	curFile->lines.push_back(0);
	hadError = false;

	if (_source != "") {
		Token token = scanToken();
		tokens.push_back(token);
		while (token.type != TOKEN_EOF) {
			token = scanToken();
			tokens.push_back(token);
		}
	}
}

vector<Token> scanner::getArr() {
	return tokens;
}

#pragma region Helpers
bool scanner::isAtEnd() {
	return current >= curFile->sourceFile.size();
}

//if matched we consume the token
bool scanner::match(char expected) {
	if (isAtEnd()) return false;
	if (curFile->sourceFile[current] != expected) return false;
	current++;
	return true;
}

char scanner::advance() {
	return curFile->sourceFile[current++];
}

Token scanner::scanToken() {
	skipWhitespace();
	start = current;

	if (isAtEnd()) return makeToken(TOKEN_EOF);

	char c = advance();
	//identifiers start with _ or [a-z][A-Z]
	if (isDigit(c)) return number();
	if (isAlpha(c)) return identifier();
	if (isPreprocessDirective(c)) return makeToken(directiveType());

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
	case ':': return makeToken(TOKEN_COLON);
	case '?': return makeToken(TOKEN_QUESTIONMARK);
	case '\n':
		curFile->lines.push_back(current);
		line++;
		return makeToken(TOKEN_NEWLINE);
	}

	return errorToken("Unexpected character.");
}

Token scanner::makeToken(TokenType type) {
	span newSpan(line, start - curFile->lines[curFile->lines.size() - 1], current - start, curFile);
	Token token(newSpan, type);
	string str = token.getLexeme();
	return token;
}

Token scanner::errorToken(const char* message) {
	return Token(message, line, TOKEN_ERROR);
}

char scanner::peek() {
	if (isAtEnd()) return '\0';
	return curFile->sourceFile[current];
}

char scanner::peekNext() {
	if (isAtEnd()) return '\0';
	return curFile->sourceFile[current + 1];
}

void scanner::skipWhitespace() {
	while (true) {
		char c = peek();
		switch (c) {
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '/':
			if (peekNext() == '/') {
				// A comment goes until the end of the line.
				while (peek() != '\n' && !isAtEnd()) advance();
			}
			else if (peekNext() == '*') {
				advance();
				while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
					if (peek() == '\n') {
						line++;
						curFile->lines.push_back(current);
					}
					advance();
				}
				if (!isAtEnd()) {
					advance();
					advance();
				}
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
		if (peek() == '\n') {
			line++;
			curFile->lines.push_back(current);
		}
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
	string& source = curFile->sourceFile;
	switch (source[start]) {
	case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
	case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
	case 'c':
		if (current - start > 1) {
			switch (source[start + 1]) {
			case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
			case 'a': return checkKeyword(2, 2, "se", TOKEN_CASE);
			}
		}
		break;
	case 'd': return checkKeyword(1, 6, "efault", TOKEN_DEFAULT);
	case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
	case 'i':
		if (current - start > 1) {
			switch (source[start + 1]) {
			case 'f': return TOKEN_IF;
			case 'm': return checkKeyword(2, 4, "port", TOKEN_IMPORT);
			}
		}
		break;
	case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
	case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
	case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
	case 'r': 
		if (current - start > 1) {
			switch (source[start + 1]) {
			case 'e': return checkKeyword(2, 4, "turn", TOKEN_RETURN);
			case 'u': return checkKeyword(2, 1, "n", TOKEN_RUN);
			}
		}
		break;
	case 's':
		if (current - start > 1) {
			switch (source[start + 1]) {
			case 'u': return checkKeyword(2, 3, "per", TOKEN_SUPER);
			case 'w': return checkKeyword(2, 4, "itch", TOKEN_SWITCH);
			}
		}
		break;
	case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
	case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
	case 'f':
		if (current - start > 1) {
			switch (source[start + 1]) {
			case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
			case 'o':
				//since foreach has the same letters as for we first check the total number of letters
				if (current - start > 3) {
					return checkKeyword(2, 5, "reach", TOKEN_FOREACH);
				}
				return checkKeyword(2, 1, "r", TOKEN_FOR);
			case 'u': return checkKeyword(2, 2, "nc", TOKEN_FUNC);
			case 'i': return checkKeyword(2, 3, "ber", TOKEN_FIBER);
			}
		}
		break;
	case 't':
		if (current - start > 1) {
			switch (source[start + 1]) {
			case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
			case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
			}
		}
		break;
	case 'y':
		return checkKeyword(1, 4, "ield", TOKEN_YIELD);
	}
	return TOKEN_IDENTIFIER;
}

//is string.compare fast enough?
TokenType scanner::checkKeyword(int strt, int length, const char* rest, TokenType type) {
	if (current - start == strt + length && curFile->sourceFile.substr(start + strt, length).compare(rest) == 0) {
		return type;
	}

	return TOKEN_IDENTIFIER;
}
#pragma endregion

#pragma region Preprocessor
bool scanner::isPreprocessDirective(char c) {
	if (c == '#') return true;
	return false;
}

TokenType scanner::directiveType() {
	while (isAlpha(peek()) || isDigit(peek())) advance();
	switch (curFile->sourceFile[start + 1]) {
	case 'm': return checkKeyword(2, 4, "acro", TOKEN_MACRO) == TOKEN_IDENTIFIER ? TOKEN_ERROR : TOKEN_MACRO;
	case 'i': return checkKeyword(2, 5, "mport", TOKEN_IMPORT) == TOKEN_IDENTIFIER ? TOKEN_ERROR : TOKEN_IMPORT;
	}
}
#pragma endregion

//name:line:column: error: msg
//line
//which part
//symbol: token.getLexeme()

const string cyan = "\u001b[38;5;117m";
const string black = "\u001b[0m";
const string red = "\u001b[38;5;196m";
const string yellow = "\u001b[38;5;220m";

void underlineToken(span symbol) {
	file* src = symbol.sourceFile;
	uInt64 lineStart = src->lines[symbol.line - 1];
	uInt64 lineEnd = 0;

	if (src->lines.size() - 1 == symbol.line - 1) lineEnd = src->sourceFile.size();
	else lineEnd = src->lines[symbol.line];

	string line = std::to_string(symbol.line);
	std::cout << yellow + src->name + black + ":" + cyan + line + " | " + black;
	std::cout << src->sourceFile.substr(lineStart, lineEnd - lineStart);

	string temp = "";
	temp.insert(0, src->name.size() + line.size() + 4, ' ');
	uInt64 tempN = 0;
	for (; tempN < symbol.column; tempN++) temp.append(" ");
	for (; tempN < symbol.column + symbol.length; tempN++) temp.append("^");

	std::cout << red + temp + black + "\n";
}

void report(file* src, Token& token, string msg) {
	if (token.type == TOKEN_EOF) {
		std::cout << "End of file. \n" << msg;
		return;
	}
	string name = "\u001b[38;5;220m" + src->name + black;
	std::cout << red + "error: " + black + msg + "\n";

	if (token.partOfMacro) {
		underlineToken(token.macro);
	}

	underlineToken(token.str);
	std::cout << "\n";
}

