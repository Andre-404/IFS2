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

int main() {
	cout << "Input filepath:\n";
	string path;
	cin >> path;
	//doing this insanity to seperate the path to the file(which is needed for includes) and the name of the file to compile
	string firstFileName;
	firstFileName = path.substr(path.find_last_of('\\') + 1, path.size() - path.find_last_of('\\'));
	firstFileName = firstFileName.substr(0, firstFileName.find(".txt"));
	path = path.substr(0, path.find_last_of('\\') + 1);
	cout << "Output:\n\n";
	if (!path.empty()) {
		compiler* comp = new compiler(path, firstFileName, funcType::TYPE_SCRIPT);
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