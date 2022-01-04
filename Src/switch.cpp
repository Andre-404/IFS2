#include "switch.h"


using std::vector;

//based on the type we create either a hash map or array on the heap(to save space), and leave the other as null
switchTable::switchTable(switchType _type, int size) {
	type = _type;
	defaultJump = -1;
	//in order to save memory, we have pointers and only create either the array or hash table
	switch (type) {
	case switchType::STRING:
		table = new std::unordered_map<string, long>;
		table->reserve(size);
		arr = NULL;
		break;
	case switchType::NUM:
		arr = new vector<switchPairNum>;
		arr->reserve(size);
		table = NULL;
		break;
	case switchType::MIXED:
		table = new std::unordered_map<string, long>;
		table->reserve(size);
		arr = NULL;
		break;
	default:
		table = NULL;
		arr = NULL;
		break;
	}
}

//insertion sort for the last element of the array
void insertSorted(vector<switchPairNum>* _arr, switchPairNum _el) {
	vector<switchPairNum>& arr = *_arr;
	int i = arr.size();
	int j = i - 1;
	switchPairNum element = _el;
	arr.push_back(element);

	while (j >= 0 && arr[j].key > element.key) {
		arr[j + 1] = arr[j];
		j--;
	}
	arr[j + 1] = element;
}

int binarySearch(vector<switchPairNum>* arr, int key, int start, int end) {
	if (end >= 1) {
		int mid = start + (end - start) / 2;
		double compareKey = arr->at(mid).key;
		if (compareKey == key) return mid;

		if (compareKey > key) return binarySearch(arr, key, start, mid - 1);

		return binarySearch(arr, key, mid + 1, end);
	}

	return -1;
}

void switchTable::addToArr(int key, long ip) {
	if (arr == NULL) exit(64);
	insertSorted(arr, switchPairNum(key, ip));
}

void switchTable::addToTable(string str, long ip) {
	if (table == NULL) exit(64);
	table->insert(std::make_pair(str, ip));
}

long switchTable::getJump(string& key) {
	if (table->find(key) == table->end()) return -1;
	return table->at(key);
}

long switchTable::getJump(int key) {
	int pos = binarySearch(arr, key, 0, arr->size() - 1);
	if (pos == -1) return -1;
	return arr->at(pos).ip;
}