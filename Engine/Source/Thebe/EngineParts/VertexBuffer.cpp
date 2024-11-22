#include "Thebe/EngineParts/VertexBuffer.h"

using namespace Thebe;

VertexBuffer::VertexBuffer()
{
}

/*virtual*/ VertexBuffer::~VertexBuffer()
{
}

/*virtual*/ bool VertexBuffer::Setup(void* data)
{
	return true;
}

/*virtual*/ void VertexBuffer::Shutdown()
{
}