#pragma once
	
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>

using std::string;

typedef unsigned int uInt;
typedef unsigned long long uInt64;
typedef unsigned short uInt16;
typedef char byte;

constexpr double pi = 3.14159265358979323846;

namespace global {
		
};
//Only use this if you think the parsing stage is incorrect
//#define DEBUG_PRINT_AST
//#define DEBUG_PRINT_CODE
//WARNING: massivly slows down code execution
//#define DEBUG_TRACE_EXECUTION
//#define DEBUG_GC
//#define DEBUG_STRESS_GC