#pragma once

#include "common.h"
#include "AST.h"
#include <unordered_map>
#include "gcVector.h"
#include "hashTable.h"


//this is for the when we have a switch full of numbers
struct switchPairNum {
	int key;
	uInt64 ip;
	switchPairNum(int _key, uInt64 _ip) : key(_key), ip(_ip) {}
};

//struct containing all info for a single switch stmt
struct switchTable {
	//long is offset in bytecode(ip)
	//optimization, use a union?
	std::unordered_map<string, uInt64> table;
	gcVector<switchPairNum> arr;
	switchType type;
	uInt64 defaultJump;
	switchTable(switchType _type, int size);
	void addToArr(int key, long ip);
	void addToTable(string str, uInt64 ip);
	long getJump(int key);
	long getJump(string& str);
};
