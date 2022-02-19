#pragma once
#include "managed.h"
#include "memory.h"


class stringHeader : public managed {
public:
	stringHeader(char* str, uInt len);
	stringHeader(uInt len);

	void* operator new(size_t size, size_t strLen);

	void move(byte* to);
	size_t getSize() { return sizeof(stringHeader) + len; }
	void updatePtrs() { raw = reinterpret_cast<char*>(reinterpret_cast<byte*>(moveTo) + sizeof(stringHeader)); }
	void trace(std::vector<managed*>& stack) {}

	char* raw;
	uInt len;
	uInt hash;
};

class gcString{
public:
	gcString();
	gcString(char* str, uInt len);


	friend bool compare(gcString& str1, gcString& str2);
	friend gcString concat(gcString& str1, gcString& str2);

	void mark();
	void update();

	//length - 1 to account for the null terminator
	uInt getLength() { return str == nullptr ? 0 : str->len - 1; }
	void print() { if(str != nullptr) std::cout << str->raw; }
private:
	stringHeader* str;
};