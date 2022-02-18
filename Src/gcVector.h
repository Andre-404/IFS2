#pragma once

#include "managed.h"
#include "memory.h"
#include <immintrin.h>

class arrHeader : public managed {
public:
	byte* arr;
	uInt capacity;
	uInt count;
	uInt sizeofType;
	arrHeader(uInt _capacity, uInt _sizeofType);

	void move(byte* to){ memmove(to, this, sizeof(arrHeader) + sizeofType * capacity); }
	size_t getSize() { return sizeof(arrHeader) + (sizeofType * capacity); }
	void updatePtrs() { arr = reinterpret_cast<byte*>(moveTo) + sizeof(arrHeader); }
	//nothing happens here because the arr follows this header can contain any element
	void trace(std::vector<managed*>& stack) {}

	void* operator new(size_t size, size_t arrSize);
};

//dynamic array that allocates on the LOH heap of the GC, deallocation of the array is handled entirely by the GC
//items inside of the array need to have a default constructor and need to be POD
template<typename T>
class gcVector{
public:
	gcVector() {
		header = nullptr;
	}

	gcVector(size_t size) {
		size_t amortizedSize = (1ll << (64 - _lzcnt_u64(size - 1)));
		header = new(amortizedSize * sizeof(T)) arrHeader(amortizedSize, sizeof(T));

		T* rawArr = reinterpret_cast<T*>(header->arr);
		for (int i = 0; i < amortizedSize; i++) {
			new(static_cast<void*>(&rawArr[i])) T();
		}
		header->count = size;
	}

	void update() {
		if (header == nullptr) return;
		header = reinterpret_cast<arrHeader*>(header->moveTo);
	}

	void mark() {
		if (header == nullptr) return;
		header->moveTo = header;
	}

	void push(const T& val) {
		size_t count = 0;
		if (header != nullptr) count = header->count;
		ensureCapacity(count + 1);
		//this is done because the rest of the code treats the pointer as a array, and to get the proper element of that array we first
		//need to cast that array to the appropriate type
		T* arr = getArr();
		arr[header->count] = val;
		header->count++;
	}

	T pop() {
		//TODO: maybe change this to also check and downsize the array?
		T* arr = getArr();
		T val = arr[header->count - 1];
		header->count--;
		return val;
	}

	void insert(const T& val, int index) {
		if (index < 0 || index > header->count) {
			std::cout << "Array access out of bounds";
			exit(64);
		}

		ensureCapacity(header->count + 1);
		T* arr = getArr();
		//moves every element above the insertion index up by 1 position
		uInt _count = header->count;
		for (int i = _count; i > index; i--) arr[i] = arr[i - 1];

		arr[index] = val;
		header->count++;
	}

	T removeAt(int index) {
		if (index < 0 || index > header->count) {
			std::cout << "Array access out of bounds";
			exit(64);
		}
		T* arr = getArr();

		T itemToRemove = arr[index];

		//downshift every element above the remove index
		uInt _count = header->count;
		for (int i = index; i < _count - 1; i++) arr[i] = arr[i + 1];

		// Clear the copy of the last item.
		arr[header->count - 1] = T();
		header->count--;

		return itemToRemove;
	}

	void clear() {
		//GC takes care of the array
		header = nullptr;
	}

	void resize(int newSize) {
		//if the array hasn't been initialized until now, create a new array and return
		if (header == nullptr) {
			size_t amortizedSize = (1ll << (64 - _lzcnt_u64(newSize - 1)));

			header = new(amortizedSize * sizeof(T)) arrHeader(amortizedSize, sizeof(T));
			T* rawArr = reinterpret_cast<T*>(header->arr);
			for (int i = 0; i < newSize; i++) {
				new(static_cast<void*>(&rawArr[i])) T();
			}
			header->count = newSize;
			return;
		}
		//if we do have enough capacity for the new size, simply update the count
		if (header->count >= newSize) {
			T* arr = getArr();
			uInt _count = header->count;
			for (int i = newSize; i < _count; i++) {
				arr[i] = T();
			}
			header->count = newSize;
		}else {
			ensureCapacity(newSize);
			T* arr = getArr();
			uInt _count = header->count;
			for (int i = _count; i < newSize; i++) {
				new(static_cast<void*>(&arr[i])) T();
			}
			header->count = newSize;
		}
	}

	void addAll(gcVector<T>& other) {
		ensureCapacity(count() + other.count());
		T* arr = getArr();

		for (int i = 0; i < other.count(); i++) arr[header->count++] = other[i];
	}

	T& operator[] (int index) {
		if (index < 0 || index > header->count) {
			std::cout << "Array access out of bounds";
			exit(64);
		}
		T* arr = getArr();
		return arr[index];
	}

	const T& operator[] (int index) const {
		if (index < 0 || index > header->count) {
			std::cout << "Array access out of bounds";
			exit(64);
		}
		T* arr = getArr();
		return arr[index];
	}
	//null checks because the default constructor doesn't allocate the header
	uInt count() { return header == nullptr ? 0 : header->count; }
	uInt capacity() { return header == nullptr ? 0 : header->capacity; }
	bool isEmpty() { return header == nullptr || header->count == 0; }
private:
	arrHeader* header;

	void ensureCapacity(size_t capacity) {
		//always use amortized capacity for better performance
		if (header == nullptr) {
			size_t newCapacity = (1ll << (64 - _lzcnt_u64(capacity - 1)));
			header = new(newCapacity * sizeof(T)) arrHeader(newCapacity, sizeof(T));
			return;
		}
		if (header->capacity >= capacity) return;
		size_t newCapacity = (1ll << (64 - _lzcnt_u64(capacity - 1)));
		arrHeader* newHeader = new(newCapacity * sizeof(T)) arrHeader(newCapacity, sizeof(T));

		memcpy(newHeader->arr, header->arr, header->count * header->sizeofType);

		newHeader->count = header->count;
		header = newHeader;
	}

	T* getArr() {
		return reinterpret_cast<T*>(header->arr);
	}
};