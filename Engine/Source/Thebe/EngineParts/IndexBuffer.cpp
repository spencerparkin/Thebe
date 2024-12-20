#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

IndexBuffer::IndexBuffer()
{
	::memset(&this->indexBufferView, 0, sizeof(this->indexBufferView));
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	this->primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	this->sizeAlignmentRequirement = 1;
}

/*virtual*/ IndexBuffer::~IndexBuffer()
{
}

void IndexBuffer::SetFormat(DXGI_FORMAT format)
{
	this->indexBufferView.Format = format;
}

void IndexBuffer::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	this->primitiveTopology = primitiveTopology;
}

D3D_PRIMITIVE_TOPOLOGY IndexBuffer::GetPrimitiveTopology() const
{
	return this->primitiveTopology;
}

const D3D12_INDEX_BUFFER_VIEW* IndexBuffer::GetIndexBufferView() const
{
	return &this->indexBufferView;
}

/*virtual*/ bool IndexBuffer::Setup()
{
	if (this->primitiveTopology == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
	{
		THEBE_LOG("No primitive topology configured.");
		return false;
	}

	if (!Buffer::Setup())
		return false;

	this->indexBufferView.BufferLocation = this->gpuBuffer->GetGPUVirtualAddress();
	this->indexBufferView.SizeInBytes = this->GetBufferSize();

	return true;
}

/*virtual*/ void IndexBuffer::Shutdown()
{
	Buffer::Shutdown();

	::memset(&this->indexBufferView, 0, sizeof(this->indexBufferView));
}

UINT IndexBuffer::GetStride() const
{
	switch (this->indexBufferView.Format)
	{
	case DXGI_FORMAT_R16_UINT:
		return 2;
	case DXGI_FORMAT_R32_UINT:
		return 4;
	}

	return 0;
}

UINT IndexBuffer::GetIndexCount() const
{
	UINT stride = this->GetStride();
	if (stride == 0)
		return 0;

	return UINT(this->originalBuffer.size()) / stride;
}

/*virtual*/ bool IndexBuffer::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!Buffer::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto formatValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("format"));
	if (!formatValue)
	{
		THEBE_LOG("No format given.");
		return false;
	}

	this->indexBufferView.Format = (DXGI_FORMAT)formatValue->GetValue();

	auto primitiveTopologyValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("primitive_topology"));
	if (!primitiveTopologyValue)
	{
		THEBE_LOG("No primitive topology given.");
		return false;
	}

	this->primitiveTopology = (D3D_PRIMITIVE_TOPOLOGY)primitiveTopologyValue->GetValue();

	return true;
}

/*virtual*/ bool IndexBuffer::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!Buffer::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	rootValue->SetValue("format", new JsonInt(this->indexBufferView.Format));
	rootValue->SetValue("primitive_topology", new JsonInt(this->primitiveTopology));

	return true;
}