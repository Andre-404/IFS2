#include "common.h"
#include <iostream>
#include <fstream>
#include "scanner.h"
#include "parser.h"
#include "VM.h"
#include "compiler.h"
#include "namespaces.h"
#include "memory.h"

using std::cout;
using std::cin;

//init the variables of global namespace
hashTable global::internedStrings = hashTable();
GC global::gc = GC();
std::mt19937 global::rng(0);


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
	cout << "Input filepath:\n";
	string path;
	cin >> path;
	string source = readFile(path);
	cout << "Output:\n\n";
	if (!source.empty()) {
		scanner* scan = new scanner(&source);
		parser* parse = new parser(scan->getArr());
		compiler* comp = new compiler(parse, funcType::TYPE_SCRIPT);
		delete scan;
		vm* newVm = new vm(comp);
		delete newVm;
	}
	while (true) {
		cin >> path;
		return 0;
	}
	cin.get();
	return 0;
}