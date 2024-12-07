#include "Thebe/Utilities/ScratchHeap.h"

using namespace Thebe;

ScratchHeap::ScratchHeap()
{
	this->offset = 0;
}

/*virtual*/ ScratchHeap::~ScratchHeap()
{
}

void ScratchHeap::SetSize(uint64_t size)
{
	this->buffer.resize(size);
}

uint64_t ScratchHeap::GetSize()
{
	return this->buffer.size();
}

void ScratchHeap::Reset()
{
	this->offset = 0;
}

uint8_t* ScratchHeap::Allocate(uint64_t size, uint64_t align)
{
	uint64_t blockStart = THEBE_ALIGNED(this->offset, align);
	uint64_t blockEnd = blockStart = size;
	if (blockEnd > this->GetSize())
		return nullptr;

	this->offset = blockEnd;
	uint8_t* block = &this->buffer.data()[blockStart];
	return block;
}