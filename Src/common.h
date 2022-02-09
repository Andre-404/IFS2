#ifndef __IFS_COMMON
	#define __IFS_COMMON
	
	#include <stdlib.h>
	#include <string>
	#include <vector>
	#include <stdbool.h>
	#include <iostream>

	using std::string;

	//used for strings
	typedef unsigned int uInt;
	typedef unsigned long long uInt64;
	//typedef unsigned long long uHash;

	namespace global {
		
	};
	//Only use this if you think the parsing stage is incorrect
	//#define DEBUG_PRINT_AST
	//WARNING: massivly slows down code execution
	#define DEBUG_PRINT_CODE
	#define DEBUG_TRACE_EXECUTION
	//#define DEBUG_GC
	//#define DEBUG_STRESS_GC
#endif