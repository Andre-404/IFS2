#include "files.h"
#include <stdio.h>


string readFile(char* path) {
	std::FILE* fp;
	errno_t err = fopen_s(&fp, path, "rb");
	if (err == 0)
	{
		std::string contents;
		std::fseek(fp, 0, SEEK_END);
		contents.resize(std::ftell(fp));
		std::rewind(fp);
		std::fread(&contents[0], 1, contents.size(), fp);
		std::fclose(fp);
		return contents;
	}
	else {
		std::cout << "Couldn't open file " << path << "\n";
		return "";
	}
}
string readFile(const char* path) {
	return readFile((char*)path);
}
string readFile(string& path) {
	return readFile(path.c_str());
}