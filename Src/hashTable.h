#ifndef __IFS_HASH_TABLE

#define __IFS_HASH_TABLE

#include "common.h"
#include "value.h"

class objString;


struct entry {
	objString* key;
	Value val;
};

#define TOMBSTONE (objString*)0x000001

class hashTable {
public:
	//this should honestly be moved to the GC managed heap
	std::vector<entry> entries;
	uInt64 count;
	uInt64 capacity;

	hashTable();
	bool set(objString* key, Value val);
	bool get(objString* key, Value* val);
	objString* getKey(Value val);
	bool del(objString* key);
	void tableAddAll(hashTable* src);
private:
	void resize(uInt64 _capacity);
	entry* findEntry(std::vector<entry> &_entries, objString* key);
};

objString* findInternedString(hashTable* table, char* str, uInt length, uInt64 hash);


#endif // !__IFS_HASH_TABLE
