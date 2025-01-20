#pragma once

#include "Thebe/Common.h"
#include <vector>

namespace Thebe
{
	/**
	 * This is a heap that provides O(1) allocate and deallocation by
	 * virtue of the fact that every allocation is the same size.
	 */
	class THEBE_API StackHeap
	{
	public:
		StackHeap();
		virtual ~StackHeap();

		bool SetManagedMemory(uint8_t* memoryBuffer, uint64_t memoryBufferSize, uint64_t blockSize);

		uint8_t* AllocateBlock();
		bool DeallocateBlock(uint8_t* block);

	protected:
		uint8_t* memoryBuffer;
		uint64_t memoryBufferSize;
		uint64_t blockSize;
		std::vector<uint64_t> blockStack;
		uint64_t blockStackTop;
	};

	/**
	 * This class offers a convenient way to utilize a stack heap
	 * to manage class instances.  Constructors and destructors
	 * are called on allocation and deallocation, respectively.
	 */
	template<typename T>
	class THEBE_API ObjectHeap : public StackHeap
	{
	public:
		ObjectHeap()
		{
		}

		virtual ~ObjectHeap()
		{
		}

		void Configure(uint8_t* memoryBuffer, uint64_t memoryBufferSize)
		{
			this->SetManagedMemory(memoryBuffer, memoryBufferSize, sizeof(T));
		}

		T* Allocate()
		{
			THEBE_ASSERT(this->blockSize >= sizeof(T));

			uint8_t* block = this->AllocateBlock();
			if (!block)
				return nullptr;

			T* object = new (block) T();
			return object;
		}

		bool Deallocate(T* object)
		{
			auto block = reinterpret_cast<uint8_t*>(object);
			if (!this->DeallocateBlock(block))
				return false;

			object->~T();
			return true;
		}
	};
}