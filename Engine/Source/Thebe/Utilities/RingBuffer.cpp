#include "Thebe/Utilities/RingBuffer.h"

using namespace Thebe;

RingBuffer::RingBuffer(uint32_t size)
{
	THEBE_ASSERT_FATAL(size > 0);
	this->bufferSize = size;
	this->buffer = new uint8_t[size];
	this->numFreeBytes = size;
	this->startOffset = 0;
	this->endOffset = 0;
}

/*virtual*/ RingBuffer::~RingBuffer()
{
	delete[] this->buffer;
}

uint32_t RingBuffer::GetNumFreeBytes() const
{
	return this->numFreeBytes;
}

uint32_t RingBuffer::GetNumStoredBytes() const
{
	return this->bufferSize - this->numFreeBytes;
}

bool RingBuffer::WriteBytes(const uint8_t* byteArray, uint32_t byteArraySize)
{
	if (byteArraySize > this->numFreeBytes)
		return false;

	for (uint32_t i = 0; i < byteArraySize; i++)
	{
		this->buffer[this->endOffset] = byteArray[i];
		this->endOffset = (this->endOffset + 1) % this->bufferSize;
	}

	this->numFreeBytes -= byteArraySize;
	return true;
}

bool RingBuffer::PeakBytes(uint8_t* byteArray, uint32_t byteArraySize)
{
	if (byteArraySize > this->GetNumStoredBytes())
		return false;

	for (uint32_t i = 0; i < byteArraySize; i++)
		byteArray[i] = this->buffer[(this->startOffset + i) % this->bufferSize];

	return true;
}

bool RingBuffer::ReadBytes(uint8_t* byteArray, uint32_t byteArraySize)
{
	if (!this->PeakBytes(byteArray, byteArraySize))
		return false;

	if (!this->DeleteBytes(byteArraySize))
		return false;

	return true;
}

bool RingBuffer::DeleteBytes(uint32_t numBytes)
{
	if (numBytes > this->GetNumStoredBytes())
		return false;

	this->startOffset += numBytes;
	if (this->startOffset >= this->bufferSize)
		this->startOffset -= this->bufferSize;

	this->numFreeBytes += numBytes;
	return true;
}