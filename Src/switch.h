#pragma once

#include "common.h"
#include "AST.h"
#include <unordered_map>
#include "gcVector.h"
#include "hashTable.h"


//this is for the when we have a switch full of numbers
struct switchPairNum {
	int key;
	long ip;
	switchPairNum(int _key, long _ip) : key(_key), ip(_ip) {}
};

//struct containing all info for a single switch stmt
struct switchTable {
	//long is offset in bytecode(ip)
	//optimization, use a union?
	std::unordered_map<string, long>* table;
	std::vector<switchPairNum> arr;
	switchType type;
	long defaultJump;
	switchTable(switchType _type, int size);
	void addToArr(int key, long ip);
	void addToTable(string str, long ip);
	long getJump(int key);
	long getJump(string& str);
};
