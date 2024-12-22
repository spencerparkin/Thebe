#include "Thebe/EngineParts/StructuredBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

StructuredBuffer::StructuredBuffer()
{
	this->structSize = 0;
}

/*virtual*/ StructuredBuffer::~StructuredBuffer()
{
}

/*virtual*/ bool StructuredBuffer::Setup()
{
	if (this->structSize == 0)
	{
		THEBE_LOG("Structure size is zero.");
		return false;
	}

	UINT64 bufferSize = this->originalBuffer.size();
	if (bufferSize == 0)
	{
		THEBE_LOG("Buffer size is zero.");
		return false;
	}

	if (bufferSize % this->structSize != 0)
	{
		THEBE_LOG("The buffer size (%ull) must be a multiple of the structure size (%ull).", bufferSize, this->structSize);
		return false;
	}

	D3D12_RESOURCE_DESC& bufferDesc = this->GetResourceDesc();
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;

	if (!Buffer::Setup())
		return false;

	return true;
}

/*virtual*/ void StructuredBuffer::Shutdown()
{
	Buffer::Shutdown();
}

/*virtual*/ bool StructuredBuffer::CreateResourceView(CD3DX12_CPU_DESCRIPTOR_HANDLE& handle, ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = (UINT)this->GetNumStructs();
	srvDesc.Buffer.StructureByteStride = this->structSize;
	device->CreateShaderResourceView(this->gpuBuffer.Get(), &srvDesc, handle);
	return true;
}

void StructuredBuffer::SetStructSize(UINT64 structSize)
{
	this->structSize = structSize;
}

UINT64 StructuredBuffer::GetStructSize() const
{
	return this->structSize;
}

UINT64 StructuredBuffer::GetNumStructs() const
{
	return UINT64(this->originalBuffer.size()) / this->structSize;
}