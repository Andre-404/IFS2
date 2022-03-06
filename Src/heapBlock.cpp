#include "heapBlock.h"
#include "value.h"
#include "hashTable.h"
#include "object.h"

void updateVal(Value* val) {
	if (val->type == VAL_OBJ) val->as.object = (obj*)(val->as.object->moveTo);
}

void updateTable(hashTable* table) {
	for (int i = 0; i < table->capacity; i++) {
		entry* _entry = &table->entries[i];
		//only update live cells(avoid empty/tombstone ones)
		if (_entry->key && _entry->key != TOMBSTONE) {
			//this little check here is for global::internedStrings, since the strings there are "weak" pointers
			if (_entry->key->str == nullptr) std::cout << "Key is null when it shouldnt be\n";
			if (_entry->key->moveTo == nullptr) {
				table->del(_entry->key);
			}
			else {
				_entry->key = reinterpret_cast<objString*>(_entry->key->moveTo);
				updateVal(&_entry->val);
			}
		}
	}
	//updates the internal ptr of the array header
	table->entries.update();
}

bool forwardAddress(managed* ptr) {
	return ptr->moveTo != nullptr;
}

movingHeapBlock::movingHeapBlock() {
	try {
		heapBuffer = new byte[HEAP_START_SIZE];
	}catch (std::bad_alloc& ba) {
		std::cout << "Couldn't allocate memory for heap, exiting...\n";
	}
	oldHeapBuffer = nullptr;
	heapTop = heapBuffer;

	heapSize = HEAP_START_SIZE;
}

void* movingHeapBlock::allocate(size_t size) {
	void* ptr = heapTop;
	heapTop += size;
	return ptr;
}

bool movingHeapBlock::canAllocate(size_t size) {
	return heapTop + size < heapBuffer + heapSize;
}

bool movingHeapBlock::canShrink(size_t size) {
	size_t sizeAfterAllocation = (heapTop + size)-heapBuffer;
	double heapTaken = ((double)sizeAfterAllocation) / ((double)heapSize);
	if (heapTaken < .25 && heapSize > HEAP_START_SIZE * 2) {
		return true;
	}
	return false;
}

void movingHeapBlock::resize(size_t size) {
	//Amortized size to reduce the number of future resizes
	size_t newHeapSize = (1ll << (64 - _lzcnt_u64(heapSize + size - 1)));
	oldHeapBuffer = heapBuffer;
	try {
		heapBuffer = new byte[newHeapSize];
	}catch (std::bad_alloc& ba) {
		std::cout << "Couldn't allocate memory to resize the heap, exiting...\n";
	}
	heapSize = newHeapSize;
}

void movingHeapBlock::shrink() {
	size_t newHeapSize = (heapSize >> 2);
	std::cout << "Heap shrank from: "<<heapSize<<" to: "<<newHeapSize<<"\n";
	oldHeapBuffer = heapBuffer;
	try {
		heapBuffer = new byte[newHeapSize];
	}
	catch (std::bad_alloc& ba) {
		std::cout << "Couldn't allocate memory to resize the heap, exiting...\n";
	}
	heapSize = newHeapSize;
}

void movingHeapBlock::clear() {
	byte* it = heapBuffer;

	//we need to explicitly call the destructor for each object to handle things like STL containers
	while (it < heapTop) {
		managed* temp = reinterpret_cast<managed*>(it);
		size_t size = temp->getSize();
		temp->~managed();
		it += size;

	}

	delete[] heapBuffer;
	heapBuffer = nullptr;
	heapTop = nullptr;
	oldHeapBuffer = nullptr;
}

void movingHeapBlock::computeAddress() {
	//we scan the heap linearly, for each marked object(those whose moveTo field isn't null), we calculate a new(compacted) position
	//"to" can point either to start of the same memory block, or to a entirely memory block
	if (oldHeapBuffer == nullptr) oldHeapBuffer = heapBuffer;
	byte* to = heapBuffer;
	byte* from = oldHeapBuffer;

	while (from < heapTop) {
		managed* temp = reinterpret_cast<managed*>(from);
		if (forwardAddress(reinterpret_cast<managed*>(from))) {
			temp->moveTo = reinterpret_cast<managed*>(to);
			//move the compacted position pointer
			to += temp->getSize();
		}
		//get the next object from old heap
		from += temp->getSize();

	}
}

void movingHeapBlock::updatePtrs() {
	//sort of like marking, we scan the heap lineraly, and for each marked object we update it's pointers
	byte* current = oldHeapBuffer == nullptr ? heapBuffer : oldHeapBuffer;

	while (current < heapTop) {
		managed* temp = reinterpret_cast<managed*>(current);
		if (forwardAddress(temp)) {
			temp->updatePtrs();
		}
		current += temp->getSize();
	}
}

void movingHeapBlock::clearFlags() {
	//sort of like marking, we scan the heap lineraly, and for each marked object we update it's pointers
	byte* current = oldHeapBuffer == nullptr ? heapBuffer : oldHeapBuffer;

	while (current < heapTop) {
		managed* temp = reinterpret_cast<managed*>(current);
		temp->moveTo = nullptr;
		current += temp->getSize();
	}
}

void movingHeapBlock::compact() {
	//heap can be either the same memory buffer, or a completely different one
	//we're always copy FROM old heap
	byte* from = oldHeapBuffer;
	byte* newTop = heapBuffer;

	//heapTop points to the top of the old heap, we ONLY update heapTop once we're done with compacting
	while (from < heapTop) {
		managed* curObject = reinterpret_cast<managed*>(from);
		size_t sizeOfObj = curObject->getSize();
		byte* nextObj = from + sizeOfObj;

		if (forwardAddress(curObject)) {
			byte* to = reinterpret_cast<byte*>(curObject->moveTo);
			curObject->moveTo = nullptr;//reset the marked flag
			//this is a simple optimization, if the object doesn't move in memory at all, there's no need to copy it
			if (from != to) curObject->move(to);
			newTop = to + sizeOfObj;
		}
		else {
			curObject->~managed();
		}
		from = nextObj;
	}
	//only update heapTop once we're done compacting
	heapTop = newTop;
	#ifdef DEBUG_GC
	//this way any unupdated pointers won't point to vaild memory that the GC considers unoccupied
	memset(heapTop, 0, heapSize - (heapTop - heapBuffer));
	#endif // DEBUG_GC
}

void movingHeapBlock::dump(bool isPost) {
	byte* from = isPost ? heapBuffer : oldHeapBuffer;
	byte* inc = from;

	std::cout << "|";
	while (inc < heapTop) {
		managed* temp = (managed*)inc;
		std::cout << "hdr: " << temp << " size: " << temp->getSize();
		inc += temp->getSize();
		std::cout << "|";
	}

	std::cout << "\nWent through: " << inc - from << " out of: " << heapSize;
	std::cout << "\n";
}


void* staticHeapBlock::allocate(size_t size){
	sinceLastClear++;
	if (freeBlocks.count > 0) {
		for (int i = 0; i < freeBlocks.count; i++) {
			if (size == freeBlocks[i].size) {
				memset(freeBlocks[i].block, 0, size);
				objects.push_back(freeBlocks[i]);
				freeBlocks.count--;
				return freeBlocks[i].block;
			}
		}
	}
	memBlock block(size);
	objects.push_back(block);
	return block.block;
}

void staticHeapBlock::clear() {
	for (int i = 0; i < objects.size(); i++) delete(objects[i].block);
	objects.clear();
}

void staticHeapBlock::clearFlags() {
	for (int i = 0; i < objects.size(); i++) objects[i].block->moveTo = nullptr;
}

void staticHeapBlock::updatePtrs() {
	for (int i = 0; i < objects.size(); i++) if(objects[i].block->moveTo != nullptr) objects[i].block->updatePtrs();
}

bool staticHeapBlock::shouldSweep() {
	if (sinceLastClear > 10) return true;
	return false;
}

void staticHeapBlock::sweep() {
	for (int i = objects.size(); i >= 0; i--) {
		if (objects[i].block->moveTo == nullptr) {
			if (freeBlocks.count < 10) {
				freeBlocks[freeBlocks.count] = objects[i];
				objects.erase(objects.begin() + i);
				freeBlocks.count++;
			}else {
				delete objects[i].block;
				objects.erase(objects.begin() + i);
			}
		}
		else objects[i].block->moveTo = nullptr;
	}
}