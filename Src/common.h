#ifndef __IFS_COMMON
	#define __IFS_COMMON
	
	#include <stdlib.h>
	#include <string>
	#include <vector>
	#include <stdbool.h>
	#include <iostream>

	using std::string;

	class hashTable;

#pragma region Ptr

	template<typename T>
	class ptr {
	private:
		T** _ptr;
	public:
		ptr(void* ptr = nullptr) {
			if (ptr != nullptr) {
				_ptr = (T**)ptr;
			}
			else _ptr = nullptr;
		}

		T& operator*() {
			return **_ptr;
		}

		T* operator->() {
			return *_ptr;
		}
		T** getPtr() {
			return _ptr;
		}

		void setPtr(void** ptr) {
			_ptr = (T**)ptr;
		}
		//returns the address(pointer) that this ptr<T> has stored
		T* getAdd() {
			return *_ptr;
		}

		operator bool() {
			if (_ptr == nullptr) return false;
			return *_ptr != nullptr;
		}

		void del() {
			delete _ptr;
		}
		
		template<typename Y>
		operator ptr<Y>() {
			if (_ptr == nullptr) return ptr<Y>();
			if (static_cast<Y*>(getAdd()) != nullptr) {
				return ptr<Y>(_ptr);
			}
			std::cout << "Couldn't cast: " << _ptr << "\n";
			exit(64);
		}
	};
	template<typename T>
	bool operator ==(ptr<T>& left, ptr<T>& right) {
		return left.getAdd() == right.getAdd();
	}
	template<typename T>
	bool operator ==(ptr<T>& left, T* right) {
		return (left.getAdd()) == right;
	}
	template<typename T>
	bool operator ==(T* left, ptr<T>& right) {
		return (right.getAdd()) == left;
	}
#pragma endregion

	namespace global {
		extern hashTable internedStrings;
	};
	//Only use this if you think the parsing stage is incorrect
	#define DEBUG_PRINT_AST
	//WARNING: massivly slows down code execution
	#define DEBUG_PRINT_CODE
	//#define DEBUG_TRACE_EXECUTION
#endif