#include "hashTable.h"
#define TABLE_LOAD_FACTOR 0.75//arbitrary, test and see what's best

hashTable::hashTable() {
	capacity = 0;
	count = 0;
}

bool hashTable::set(objString* key, Value val) {
	//adjust array size based on load factor, array size should be power of 2 for best performance
	if (count + 1 >= capacity*TABLE_LOAD_FACTOR) {
		//arbitrary value, should be tested and changed
		resize(((capacity) < 8 ? 8 : (capacity) * 2));
	}
	entry* _entry = findEntry(entries, key);
	bool isNewKey = _entry->key == NULL;
	if (isNewKey && IS_NIL(_entry->val)) count++;

	_entry->key = key;
	_entry->val = val;
	return isNewKey;
}

bool hashTable::get(objString* key, Value* val) {
	if (count == 0) return false;

	entry* _entry = findEntry(entries, key);
	if (_entry->key == NULL) return false;//failed to find key

	*val = _entry->val;
	return true;
}

bool hashTable::del(objString* key) {
	if (count == 0) return false;

	// Find the entry.
	entry* _entry = findEntry(entries, key);
	if (_entry->key == NULL) return false;

	// Place a tombstone in the entry.
	_entry->key = NULL;
	_entry->val = BOOL_VAL(true);
	return true;
}


entry* hashTable::findEntry(std::vector<entry> &_entries, objString* key) {
	unsigned long long index = key->hash % capacity;
	entry* tombstone = NULL;
	//loop until we either find the key we're looking for, or a open slot
	while(true) {
		entry* _entry = &_entries[index];
		if (_entry->key == NULL) {
			if (IS_NIL(_entry->val)) {
				// Empty entry.
				return tombstone != NULL ? tombstone : _entry;
			}
			else {
				// We found a tombstone.
				if (tombstone == NULL) tombstone = _entry;
			}
		}
		else if (_entry->key == key) {
			// We found the key.
			return _entry;
		}
		//make sure to loop back to the start of the array if we exceed the array length
		index = (index + 1) % capacity;
	}
}

void hashTable::resize(int _capacity) {
	//create new array and fill it with NULL
	std::vector<entry> newEntries;
	newEntries.resize(_capacity, { NULL, NIL_VAL()});
	count = 0;
	//copy over the entries of the old array into the new one, avoid tombstones and recalculate the index
	for (int i = 0; i < capacity; i++) {
		entry* _entry = &entries[i];
		if (_entry->key == NULL) continue;

		entry* dest = findEntry(newEntries, _entry->key);
		dest->key = _entry->key;
		dest->val = _entry->val;
		count++;
	}

	entries = newEntries;
	capacity = _capacity;
}

void hashTable::tableAddAll(hashTable* src) {
	for (int i = 0; i < src->capacity; i++) {
		entry* _entry = &src->entries[i];
		if (_entry->key != NULL) {
			set(_entry->key, _entry->val);
		}
	}
}

objString* findInternedString(hashTable* table, string& str, unsigned long long hash) {
	if (table->count == 0) return NULL;

	unsigned long long index = hash % table->capacity;
	int length = str.length();
	while(true) {
		entry* _entry = &table->entries[index];
		if (_entry->key == NULL) {
			// Stop if we find an empty non-tombstone entry.
			if (IS_NIL(_entry->val)) return NULL;
		}
		else if (_entry->key->hash == hash && _entry->key->str.length() == length && _entry->key->str.compare(str) == 0) {
			// We found it.
			return _entry->key;
		}
		//make sure to loop back to the start of the array if we exceed the array length
		index = (index + 1) % table->capacity;
	}
}