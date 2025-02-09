#pragma once

#include "Thebe/Common.h"

namespace Thebe
{
	/**
	 * This is a heap that provides O(1) allocation time, and does not bother
	 * with deallocation.  Rather, at the end of a frame (for example), the
	 * entire scratch heap is just reset.
	 */
	class THEBE_API ScratchHeap
	{
	public:
		ScratchHeap();
		virtual ~ScratchHeap();

		void SetSize(uint64_t size);
		uint64_t GetSize();

		void Reset();
		uint8_t* Allocate(uint64_t size, uint64_t align);

	private:
		std::vector<uint8_t> buffer;
		uint64_t offset;
	};

	/**
	 * This class offers a convenient way to utilize a scratch heap
	 * to manage class instances.  Note that only constructors are
	 * called, and destructors will never get called!
	 */
	template<typename T>
	class THEBE_API ObjectScratchHeap : public ScratchHeap
	{
	public:
		ObjectScratchHeap()
		{
		}

		virtual ~ObjectScratchHeap()
		{
		}

		T* AllocateObject(uint64_t align = 1)
		{
			uint8_t* memoryBlock = this->Allocate(sizeof(T), align);
			if (!memoryBlock)
				return nullptr;

			T* object = new (memoryBlock) T();
			return object;
		}
	};
}