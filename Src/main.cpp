#include "common.h"
#include <iostream>
#include <fstream>
#include "scanner.h"

using std::cout;
using std::cin;


string readFile(string path) {
	FILE* source;
	errno_t err = fopen_s(&source, path.c_str(), "rb");
	if (err != 0) {
		fprintf(stderr, "Could not open file \"%s\".\n", path.c_str());
		return "";
	}
	fseek(source, 0L, SEEK_END);
	size_t fileSize = ftell(source);
	rewind(source);

	char* buffer = (char*)malloc(fileSize + 1);
	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path.c_str());
		exit(74);
	}
	size_t bytesRead = fread(buffer, sizeof(char), fileSize, source);
	buffer[bytesRead] = '\0';

	fclose(source);
	string s(buffer);
	free(buffer);
	return s;
}
int main() {
	string path;
	cin >> path;
	string source = readFile(path);
	if (!source.empty()) {
		scanner scan(&source);
	}
	int a;
	cin >> a;
	return 0;
}