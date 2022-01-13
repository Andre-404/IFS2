#ifndef __IFS_HASH_TABLE

#define __IFS_HASH_TABLE

#include "common.h"
#include "object.h"



struct entry {
	objString* key;
	Value val;
};

class hashTable {
public:
	std::vector<entry> entries;
	int count;
	int capacity;
	hashTable();
	bool set(objString* key, Value val);
	bool get(objString* key, Value* val);
	objString* getKey(Value val);
	bool del(objString* key);
	void tableAddAll(hashTable* src);
private:
	void resize(int _capacity);
	entry* findEntry(std::vector<entry> &_entries, objString* _key);
};

objString* findInternedString(hashTable* table, string& str, unsigned long long hash);


#endif // !__IFS_HASH_TABLE
