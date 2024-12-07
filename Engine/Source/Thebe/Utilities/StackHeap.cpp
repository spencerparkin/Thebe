#include "Thebe/Utilities/StackHeap.h"
#include "Thebe/Log.h"

using namespace Thebe;

StackHeap::StackHeap()
{
	this->memoryBuffer = nullptr;
	this->memoryBufferSize = 0;
	this->blockSize = 0;
	this->blockStackTop = 0;
}

/*virtual*/ StackHeap::~StackHeap()
{
}

bool StackHeap::SetManagedMemory(uint8_t* memoryBuffer, uint64_t memoryBuffersize, uint64_t blockSize)
{
	this->memoryBuffer = memoryBuffer;
	this->memoryBufferSize = memoryBufferSize;
	this->blockSize = blockSize;

	if (this->blockSize > this->memoryBufferSize)
	{
		THEBE_LOG("The block size must not exceed that of the buffer.");
		return false;
	}

	if (this->memoryBufferSize % this->blockSize != 0)
	{
		THEBE_LOG("The memory buffer size must be a multiple of the block size.");
		return false;
	}

	this->blockStack.clear();
	uint64_t numBlocks = this->memoryBufferSize / this->blockSize;
	this->blockStack.resize(numBlocks);
	for (uint64_t i = 0; i < numBlocks; i++)
		this->blockStack[i] = i * blockSize;

	this->blockStackTop = 0;
	return true;
}

uint8_t* StackHeap::AllocateBlock()
{
	if (this->blockStackTop == (uint64_t)this->blockStack.size())
	{
		THEBE_LOG("Stack overflow!  No more memory can be allocated from the stack heap.");
		return nullptr;
	}

	uint64_t blockOffset = this->blockStack[this->blockStackTop++];
	uint8_t* block = &this->memoryBuffer[blockOffset];
	return block;
}

bool StackHeap::DeallocateBlock(uint8_t* block)
{
	uint64_t blockOffset = this->memoryBuffer - block;
	if (blockOffset + this->blockSize > this->memoryBufferSize || blockOffset % this->blockSize != 0)
	{
		THEBE_LOG("Invalid pointer (0x%016x) given to stack heap.", uintptr_t(block));
		return false;
	}

	if (this->blockStackTop == 0)
	{
		THEBE_LOG("Stack underflow!  All memory is already free in the stack heap.");
		return false;
	}

	this->blockStack[--this->blockStackTop] = blockOffset;
	return true;
}