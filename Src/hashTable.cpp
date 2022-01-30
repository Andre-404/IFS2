#include "hashTable.h"
#include "object.h"
#define TABLE_LOAD_FACTOR 0.75//arbitrary, test and see what's best


hashTable::hashTable() {
	capacity = 0;
	count = 0;
}

bool hashTable::set(objString* key, Value val) {
	//adjust array size based on load factor, array size should be power of 2 for best performance
	key->moveTo = key;
	if (count + 1 >= capacity*TABLE_LOAD_FACTOR) {
		//arbitrary value, should be tested and changed
		resize(((capacity) < 8 ? 8 : (capacity) * 2));
	}
	entry* _entry = findEntry(entries, key);
	bool isNewKey = _entry->key == nullptr || _entry->key == TOMBSTONE;
	if (isNewKey && _entry->key == nullptr) count++;

	_entry->key = key;
	_entry->val = val;
	key->moveTo = nullptr;
	return isNewKey;
}


bool hashTable::get(objString* key, Value* val) {
	if (count == 0) return false;

	entry* _entry = findEntry(entries, key);
	if (_entry->key == nullptr || _entry->key == TOMBSTONE) return false;//failed to find key

	*val = _entry->val;
	return true;
}


bool hashTable::del(objString* key) {
	if (count == 0) return false;

	// Find the entry.
	entry* _entry = findEntry(entries, key);
	if (_entry->key == nullptr || _entry->key == TOMBSTONE) return false;

	// Place a tombstone in the entry.
	_entry->key = TOMBSTONE;
	_entry->val = NIL_VAL();
	return true;
}

entry* hashTable::findEntry(std::vector<entry> &_entries, objString* key) {
	size_t bitMask = _entries.size() - 1 ;
	uHash index = key->hash & bitMask;
	entry* tombstone = nullptr;
	//loop until we either find the key we're looking for, or a open slot
	while(true) {
		entry* _entry = &_entries[index];
		if (_entry->key == nullptr) {//empty entry
			return tombstone != nullptr ? tombstone : _entry;
		}
		else if (_entry->key == TOMBSTONE) {//we found a tombstone entry
			if (tombstone == nullptr) tombstone = _entry;
		}
		else if (_entry->key == key) {
			// We found the key.
			return _entry;
		}
		//make sure to loop back to the start of the array if we exceed the array length
		index = (index + 1) & bitMask;
	}
}

void hashTable::resize(int _capacity) {
	//create new array and fill it with NULL
	std::vector<entry> newEntries;
	newEntries.resize(_capacity, { nullptr, Value()});
	count = 0;
	//copy over the entries of the old array into the new one, avoid tombstones and recalculate the index
	for (int i = 0; i < capacity; i++) {
		entry* _entry = &entries[i];
		if (_entry->key == nullptr || _entry->key == TOMBSTONE) continue;

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
		if (_entry->key != nullptr && _entry->key != TOMBSTONE) {
			set(_entry->key, _entry->val);
		}
	}
}

objString* hashTable::getKey(Value val) {
	for (auto it = entries.begin(); it != entries.end(); ++it) {
		if (valuesEqual(it->val, val)) return it->key;
	}
	return nullptr;
}


objString* findInternedString(hashTable* table, char* str, uInt length, uHash hash) {
	if (table->count == 0) return nullptr;
	size_t bitMask = table->capacity - 1;
	uHash index = hash & bitMask;
	while(true) {
		entry* _entry = &table->entries[index];
		if (_entry->key == nullptr) {
			// Stop if we find an empty non-tombstone entry.
			return nullptr;
		}
		else if (_entry->key != TOMBSTONE && _entry->key->hash == hash && _entry->key->length == length && _entry->key->compare(str, length)) {
			// We found it.
			return _entry->key;
		}
		//make sure to loop back to the start of the array if we exceed the array length
		index = (index + 1) & bitMask;
	}
}