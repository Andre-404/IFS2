#pragma once
#include "hashTable.h"
#include "memory.h"
#include "object.h"
#include <random>
#include <chrono>


//We use this to keep track of memory usage, this is also used by GC to determine if it's time to collect
namespace global {
	extern hashTable internedStrings;
	extern GC gc;
	extern std::mt19937 rng; // Random number generator, used by the native functions for generating random numbers
};