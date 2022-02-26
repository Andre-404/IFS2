#pragma once

#include "common.h"
#include "value.h"
#include "gcVector.h"

class objString;


struct entry {
	objString* key;
	Value val;
	entry() {
		key = nullptr;
		val = NIL_VAL();
	}
};

#define TOMBSTONE (objString*)0x000001

class hashTable {
public:
	//this should honestly be moved to the GC managed heap
	gcVector<entry> entries;
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
	entry* findEntry(gcVector<entry> &_entries, objString* key);
};

objString* findInternedString(hashTable* table, char* str, uInt length, uInt64 hash);

