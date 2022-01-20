#pragma once
#include "hashTable.h"
#include "memory.h"
#include "object.h"


//We use this to keep track of memory usage, this is also used by GC to determine if it's time to collect
namespace global {
	extern long long memoryUsage;
	extern hashTable internedStrings;
	extern GC gc;
};