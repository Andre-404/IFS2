#include "switch.h"
#include "object.h"

//based on the type we create either a hash map or array on the heap(to save space), and leave the other as null
switchTable::switchTable(switchType _type, int size) {
	type = _type;
	defaultJump = -1;
}

//insertion sort for the last element of the array
void insertSorted(gcVector<switchPairNum>& arr, switchPairNum _el) {
	int i = arr.count();
	int j = i - 1;
	switchPairNum element = _el;
	arr.push(element);

	while (j >= 0 && arr[j].key > element.key) {
		arr[j + 1] = arr[j];
		j--;
	}
	arr[j + 1] = element;
}

int binarySearch(gcVector<switchPairNum>& arr, int key) {
	uInt64 l = 0;
	uInt64 r = arr.count();

	while (l <= r) {
		uInt64 mid = l + ((r - l) / 2);
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

void switchTable::addToTable(string str, uInt64 ip) {
	table.insert(std::make_pair(str, ip));
}

long switchTable::getJump(string& str) {
	if (table.find(str) == table.end()) return -1;
	return table[str];
}

long switchTable::getJump(int key) {
	int pos = binarySearch(arr, key);
	if (pos == -1) return -1;
	return arr[pos].ip;
}