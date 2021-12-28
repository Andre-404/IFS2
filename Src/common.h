#ifndef __IFS_COMMON
	#define __IFS_COMMON
	
	#include <stdlib.h>
	#include <string>
	#include <vector>
	#include <stdbool.h>
	#include <iostream>

	using std::string;

	struct memoryTracker {
		uint32_t memoryUsage = 0;
	};

	#define DEBUG_PRINT_AST
	//WARNING: massivly slows down code execution
	#define DEBUG_PRINT_CODE
	#define DEBUG_TRACE_EXECUTION
#endif