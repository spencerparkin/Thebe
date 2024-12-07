#pragma once

#include "Thebe/Common.h"

namespace Thebe
{
	/**
	 * 
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
}