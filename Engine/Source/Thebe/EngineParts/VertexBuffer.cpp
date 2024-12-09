#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

VertexBuffer::VertexBuffer()
{
	::memset(&this->vertexBufferView, 0, sizeof(this->vertexBufferView));
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	this->sizeAlignmentRequirement = 1;
	this->semanticNameHeap.SetSize(5 * 1024);
}

/*virtual*/ VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::SetStride(UINT32 stride)
{
	this->vertexBufferView.StrideInBytes = stride;
}

const D3D12_VERTEX_BUFFER_VIEW* VertexBuffer::GetVertexBufferView() const
{
	return &this->vertexBufferView;
}

/*virtual*/ bool VertexBuffer::Setup()
{
	if (this->elementDescArray.size() == 0)
	{
		THEBE_LOG("Element description array not configured.");
		return false;
	}

	if (this->vertexBufferView.StrideInBytes == 0)
	{
		THEBE_LOG("Stride not configured.");
		return false;
	}

	if (!Buffer::Setup())
		return false;

	this->vertexBufferView.BufferLocation = this->gpuBuffer->GetGPUVirtualAddress();
	this->vertexBufferView.SizeInBytes = this->GetBufferSize();

	return true;
}

/*virtual*/ void VertexBuffer::Shutdown()
{
	Buffer::Shutdown();

	::memset(&this->vertexBufferView, 0, sizeof(this->vertexBufferView));

	this->elementDescArray.clear();
}

std::vector<D3D12_INPUT_ELEMENT_DESC>& VertexBuffer::GetElementDescArray()
{
	return this->elementDescArray;
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& VertexBuffer::GetElementDescArray() const
{
	return this->elementDescArray;
}

/*virtual*/ bool VertexBuffer::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!Buffer::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto strideValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("stride"));
	if (!strideValue)
	{
		THEBE_LOG("No stride given.");
		return false;
	}

	this->vertexBufferView.StrideInBytes = (UINT)strideValue->GetValue();

	auto elementDescArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("element_desc_array"));
	if (!elementDescArrayValue)
	{
		THEBE_LOG("No element description array given.");
		return false;
	}

	this->semanticNameHeap.Reset();
	this->elementDescArray.clear();
	for (int i = 0; i < elementDescArrayValue->GetSize(); i++)
	{
		auto elementDescValue = dynamic_cast<const JsonObject*>(elementDescArrayValue->GetValue(i));
		if (!elementDescValue)
		{
			THEBE_LOG("Expected each element description array entry to be an object.");
			return false;
		}

		D3D12_INPUT_ELEMENT_DESC elementDesc{};

		auto alignedByteOffsetValue = dynamic_cast<const JsonInt*>(elementDescValue->GetValue("aligned_byte_offset"));
		if (alignedByteOffsetValue)
			elementDesc.AlignedByteOffset = alignedByteOffsetValue->GetValue();

		auto formatValue = dynamic_cast<const JsonInt*>(elementDescValue->GetValue("format"));
		if (formatValue)
			elementDesc.Format = (DXGI_FORMAT)formatValue->GetValue();

		auto inputSlotValue = dynamic_cast<const JsonInt*>(elementDescValue->GetValue("input_slot"));
		if (inputSlotValue)
			elementDesc.InputSlot = inputSlotValue->GetValue();

		auto inputSlotClassValue = dynamic_cast<const JsonInt*>(elementDescValue->GetValue("input_slot_class"));
		if (inputSlotClassValue)
			elementDesc.InputSlotClass = (D3D12_INPUT_CLASSIFICATION)inputSlotClassValue->GetValue();

		auto instanceDataStepRateValue = dynamic_cast<const JsonInt*>(elementDescValue->GetValue("instance_data_step_rate"));
		if (instanceDataStepRateValue)
			elementDesc.InstanceDataStepRate = instanceDataStepRateValue->GetValue();

		auto semanticIndexValue = dynamic_cast<const JsonInt*>(elementDescValue->GetValue("semantic_index"));
		if (semanticIndexValue)
			elementDesc.SemanticIndex = semanticIndexValue->GetValue();

		auto semanticNameValue = dynamic_cast<const JsonString*>(elementDescValue->GetValue("semantic_name"));
		if (semanticNameValue)
		{
			std::string semanticName = semanticNameValue->GetValue();
			char* semanticNameBuffer = (char*)this->semanticNameHeap.Allocate(semanticName.length() + 1, 1);
			strcpy(semanticNameBuffer, semanticName.c_str());
			elementDesc.SemanticName = semanticNameBuffer;
		}

		this->elementDescArray.push_back(elementDesc);
	}

	return true;
}

/*virtual*/ bool VertexBuffer::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!Buffer::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	rootValue->SetValue("stride", new JsonInt(this->vertexBufferView.StrideInBytes));

	auto elementDescArrayValue = new JsonArray();
	rootValue->SetValue("element_desc_array", elementDescArrayValue);

	for (const D3D12_INPUT_ELEMENT_DESC& elementDesc : this->elementDescArray)
	{
		auto elementDescValue = new JsonObject();
		elementDescArrayValue->PushValue(elementDescValue);

		elementDescValue->SetValue("semantic_name", new JsonString(elementDesc.SemanticName));
		elementDescValue->SetValue("semantic_index", new JsonInt(elementDesc.SemanticIndex));
		elementDescValue->SetValue("format", new JsonInt(elementDesc.Format));
		elementDescValue->SetValue("input_slot", new JsonInt(elementDesc.InputSlot));
		elementDescValue->SetValue("aligned_byte_offset", new JsonInt(elementDesc.AlignedByteOffset));
		elementDescValue->SetValue("input_slot_class", new JsonInt(elementDesc.InputSlotClass));
		elementDescValue->SetValue("instance_data_step_rate", new JsonInt(elementDesc.InstanceDataStepRate));
	}

	return true;
}