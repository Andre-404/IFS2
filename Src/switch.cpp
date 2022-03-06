#include "switch.h"
#include "object.h"

//based on the type we create either a hash map or array on the heap(to save space), and leave the other as null
switchTable::switchTable(switchType _type, int size) {
	type = _type;
	defaultJump = -1;
	//in order to save memory, we have pointers and only create either the array or hash table
	switch (type) {
	case switchType::STRING:
		table = new std::unordered_map<string, long>;
		table->reserve(size);
		break;
	case switchType::NUM:
		arr.reserve(size);
		table = NULL;
		break;
	case switchType::MIXED:
		table = new std::unordered_map<string, long>;
		table->reserve(size);
		break;
	default:
		table = NULL;
		break;
	}
}

//insertion sort for the last element of the array
void insertSorted(vector<switchPairNum>& arr, switchPairNum _el) {
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

int binarySearch(vector<switchPairNum>& arr, int key) {
	int l = 0;
	int r = arr.size();

	while (l <= r) {
		int mid = (l + r) / 2;
		if (arr[mid].key < key) {
			l = mid + 1;
		}
		else if (arr[mid].key > key) {
			r = mid - 1;
		}
		else return mid;
	}
	return -1;
}

void switchTable::addToArr(int key, long ip) {
	insertSorted(arr, switchPairNum(key, ip));
}

void switchTable::addToTable(string str, long ip) {
	if (table == NULL) exit(64);
	table->insert(std::make_pair(str, ip));
}

long switchTable::getJump(string& str) {
	if (table->find(str) == table->end()) return -1;
	return table->at(str);
}

long switchTable::getJump(int key) {
	int pos = binarySearch(arr, key);
	if (pos == -1) return -1;
	return arr[pos].ip;
}