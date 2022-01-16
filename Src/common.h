#ifndef __IFS_COMMON
	#define __IFS_COMMON
	
	#include <stdlib.h>
	#include <string>
	#include <vector>
	#include <stdbool.h>
	#include <iostream>

	using std::string;

	namespace global {

	};
	//Only use this if you think the parsing stage is incorrect
	#define DEBUG_PRINT_AST
	//WARNING: massivly slows down code execution
	#define DEBUG_PRINT_CODE
	#define DEBUG_TRACE_EXECUTION
#endif