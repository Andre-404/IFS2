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

string dirPath(string path) {
	string mainFileName = path.substr(path.find_last_of('\\') + 1, path.size() - path.find_last_of('\\'));
	if (mainFileName.find(".ifs") == mainFileName.npos) {
		std::cout << "File doesn't contain the correct(.ifs) extension.\n";
		exit(64);
	}
	mainFileName = mainFileName.substr(0, mainFileName.find(".ifs"));
	if (mainFileName != "main") {
		std::cout << "Target file needs to be called \"main.ifs\", got file \""<<mainFileName<<"\"\n";
	}
	return path.substr(0, path.find_last_of('\\') + 1);
}

int main() {
	cout << "Input filepath:\n";
	string path;
	cin >> path;
	cout << "Output:\n\n";
	if (!path.empty()) {
		compiler* comp = new compiler(dirPath(path), "main", funcType::TYPE_SCRIPT);
		vm newVm(comp);
	}
	system("Pause");
	return 0;
}