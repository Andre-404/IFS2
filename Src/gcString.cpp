#include "gcString.h"
#include "namespaces.h"

//FNV-1a hash algo
static uInt hashString(char* str, uInt length) {
	uInt hash = 2166136261;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)str[i];
		hash *= 16777619;
	}
	return hash;
}

stringHeader::stringHeader(char* str, uInt _len) {
	//memory of 'str' has to be managed outside of this call
	byte* to = reinterpret_cast<byte*>(this) + sizeof(stringHeader);
	memcpy(to, str, len);

	raw = to;
	len = _len;
	hash = hashString(raw, len - 1);
	moveTo = nullptr;
}

stringHeader::stringHeader(uInt _len) {
	raw = nullptr;
	len = _len;
	hash = 0;
	moveTo = nullptr;
}

void* stringHeader::operator new(size_t size, size_t stringLength) {
	return global::gc.allocRaw(size + stringLength, true);
}

void stringHeader::move(byte* to) {
	memmove(to, this, sizeof(stringHeader) + len);
}



gcString::gcString() {
	str = nullptr;
}

gcString::gcString(char* _str, uInt len) {
	str = new(len) stringHeader(_str, len);
}

void gcString::mark() {
	if (str == nullptr || str->moveTo != nullptr) return;
	str->moveTo = str;
}

void gcString::update() {
	str = reinterpret_cast<stringHeader*>(str->moveTo);
}

bool compare(gcString& str1, gcString& str2) {
	if (str1.getLength() != str2.getLength()) return false;
	return memcmp(str1.str->raw, str2.str->raw, str1.getLength()) == 0;
}

gcString concat(gcString& str1, gcString& str2) {
	//+ 1 for null terminator
	uInt newLen = str1.getLength() + str2.getLength() + 1;
	//there is probably a more efficient way of doing this, but doing it this way allows us to NOT have to update the strings
	//when allocating the new string header
	char* _newStr = new char[newLen];
	memcpy(_newStr, str1.str->raw, str1.getLength());
	memcpy(_newStr + str1.getLength(), str2.str->raw, str2.getLength() + 1);

	//we allocate enough space to store the string, but don't actually copy any contents to it
	stringHeader* newStr = new(newLen) stringHeader(_newStr, newLen);
	delete[] _newStr;

	gcString wrapperStr;
	wrapperStr.str = newStr;
	return wrapperStr;
}


