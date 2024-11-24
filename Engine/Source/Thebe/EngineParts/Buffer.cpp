#include "Thebe/EngineParts/Buffer.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/CommandExecutor.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include "Thebe/Math/PolygonMesh.h"
#include <d3dx12.h>

using namespace Thebe;

Buffer::Buffer()
{
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_COMMON;
	this->bufferSize = 0;
	this->cpuBufferPtr = nullptr;
	this->gpuBufferPtr = nullptr;
	this->type = Type::STATIC;
}

/*virtual*/ Buffer::~Buffer()
{
	delete[] this->cpuBufferPtr;
}

/*virtual*/ bool Buffer::Setup(void* data)
{
	if (this->bufferSize > 0)
	{
		THEBE_LOG("Buffer already setup.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	if (!device)
		return false;

	auto bufferSetupData = static_cast<BufferSetupData*>(data);
	this->type = bufferSetupData->type;
	this->bufferSize = bufferSetupData->bufferSize;
	if (this->bufferSize == 0)
	{
		THEBE_LOG("Can't create buffer of size zero.");
		return false;
	}

	UINT32 i = (this->type == Type::DYNAMIC_N_BUFFER_METHOD) ? THEBE_NUM_SWAP_FRAMES : 1;
	auto slowMemBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(this->bufferSize * i);
	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	HRESULT result = device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&slowMemBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&this->slowMemBuffer));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create buffer resource in slow GPU memory.  Error: 0x%08x", result);
		return false;
	}

	if (this->type == Type::DYNAMIC_N_BUFFER_METHOD)
	{
		this->cpuBufferPtr = new UINT8[this->bufferSize];
		::memcpy(this->cpuBufferPtr, bufferSetupData->bufferData, this->bufferSize);
	}

	CD3DX12_RANGE readRange(0, 0);
	result = this->slowMemBuffer->Map(0, &readRange, reinterpret_cast<void**>(&this->gpuBufferPtr));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to map buffer into CPU memory.  Error: 0x%08x", result);
		return false;
	}

	if (this->type == Type::STATIC)
	{
		::memcpy(this->gpuBufferPtr, bufferSetupData->bufferData, this->bufferSize);
		this->slowMemBuffer->Unmap(0, nullptr);
		this->gpuBufferPtr = nullptr;
	}

	if (this->type == Type::STATIC || this->type == Type::DYNAMIC_BARRIER_METHOD)
	{
		auto fastMemBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(this->bufferSize);
		CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
		result = device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&fastMemBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&this->fastMemBuffer));
		if (FAILED(result))
		{
			THEBE_LOG("Failed to create buffer resource in fast GPU memory.  Error: 0x%08x", result);
			return false;
		}
	
		CommandExecutor* commandExecutor = graphicsEngine->GetCommandExecutor();
		if (!commandExecutor)
			return false;

		ComPtr<ID3D12GraphicsCommandList> commandList;
		if(!commandExecutor->BeginRecording(commandList))
		{
			THEBE_LOG("Failed to start command-list recording.");
			return false;
		}
		
		commandList->CopyBufferRegion(this->fastMemBuffer.Get(), 0, this->slowMemBuffer.Get(), 0, this->bufferSize);

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->fastMemBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, this->resourceStateWhenRendering);
		commandList->ResourceBarrier(1, &barrier);

		if (this->type == Type::STATIC)
		{
			D3D12_DISCARD_REGION region{};
			region.NumSubresources = 1;
			commandList->DiscardResource(this->slowMemBuffer.Get(), &region);
		}

		if (!commandExecutor->EndRecording(commandList))
			return false;

		commandExecutor->Execute();
	}

	if (this->type == Type::STATIC)
		this->slowMemBuffer = nullptr;

	return true;
}

/*virtual*/ void Buffer::Shutdown()
{
	delete[] this->cpuBufferPtr;
	this->slowMemBuffer = nullptr;
	this->fastMemBuffer = nullptr;
	this->cpuBufferPtr = nullptr;
	this->gpuBufferPtr = nullptr;
	this->bufferSize = 0;
	this->type = Type::STATIC;
}

/*virtual*/ bool Buffer::LoadFromJson(const ParseParty::JsonValue* jsonValue)
{
	return false;
}

/*virtual*/ bool Buffer::DumpToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const
{
	return false;
}

bool Buffer::Update(ID3D12GraphicsCommandList* commandList)
{
	switch (this->type)
	{
		case Type::STATIC:
		{
			THEBE_LOG("You cannot update a static buffer.");
			return false;
		}
		case Type::DYNAMIC_BARRIER_METHOD:
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->fastMemBuffer.Get(), this->resourceStateWhenRendering, D3D12_RESOURCE_STATE_COPY_DEST);
			commandList->ResourceBarrier(1, &barrier);

			commandList->CopyBufferRegion(this->fastMemBuffer.Get(), 0, this->slowMemBuffer.Get(), 0, this->bufferSize);

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->fastMemBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, this->resourceStateWhenRendering);
			commandList->ResourceBarrier(1, &barrier);

			return true;
		}
		case Type::DYNAMIC_N_BUFFER_METHOD:
		{
			if (!this->gpuBufferPtr || !this->cpuBufferPtr)
				return false;

			Reference<GraphicsEngine> graphicsEngine;
			if (!this->GetGraphicsEngine(graphicsEngine))
				return false;

			SwapChain* swapChain = graphicsEngine->GetSwapChain();
			if (!swapChain)
				return false;

			UINT32 i = swapChain->GetCurrentBackBufferIndex();
			UINT32 offset = i * this->bufferSize;
			::memcpy(&this->gpuBufferPtr[offset], this->cpuBufferPtr, this->bufferSize);

			return true;
		}
	}

	return false;
}

UINT8* Buffer::GetBufferPtr()
{
	switch (this->type)
	{
	case Type::DYNAMIC_BARRIER_METHOD:
		return this->gpuBufferPtr;
	case Type::DYNAMIC_N_BUFFER_METHOD:
		return this->cpuBufferPtr;
	}

	return nullptr;
}

UINT32 Buffer::GetBufferSize()
{
	return this->bufferSize;
}

Buffer::Type Buffer::GetBufferType()
{
	return this->type;
}

/*static*/ bool Buffer::GenerateIndexAndVertexBuffersForConvexHull(
	const std::vector<Vector3>& pointArray,
	GraphicsEngine* graphicsEngine,
	Reference<IndexBuffer>& indexBuffer,
	Reference<VertexBuffer>& vertexBuffer)
{
	PolygonMesh polygonMesh;
	if (!polygonMesh.GenerateConvexHull(pointArray))
		return false;

	const std::vector<PolygonMesh::Polygon>& polygonArray = polygonMesh.GetPolygonArray();
	for (const auto& polygon : polygonArray)
		if (polygon.vertexArray.size() != 3)
			return false;

	indexBuffer.Set(new IndexBuffer());
	indexBuffer->SetGraphicsEngine(graphicsEngine);

	std::vector<uint16_t> indexBufferData;
	indexBufferData.resize(polygonArray.size() * 3);

	int i = 0;
	for (const auto& polygon : polygonArray)
		for (int j = 0; j < (int)polygon.vertexArray.size(); j++)
			indexBufferData[i++] = (uint16_t)polygon.vertexArray[j];

	IndexBuffer::IndexBufferSetupData indexBufferSetupData;
	indexBufferSetupData.bufferSetupData.type = IndexBuffer::Type::STATIC;
	indexBufferSetupData.bufferSetupData.bufferData = (uint8_t*)indexBufferData.data();
	indexBufferSetupData.bufferSetupData.bufferSize = indexBufferData.size() * sizeof(uint16_t);
	indexBufferSetupData.format = DXGI_FORMAT_R16_UINT;
	indexBufferSetupData.primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	if (!indexBuffer->Setup(&indexBufferSetupData))
	{
		THEBE_LOG("Failed to setup index buffer.");
		return false;
	}

	vertexBuffer.Set(new VertexBuffer());
	vertexBuffer->SetGraphicsEngine(graphicsEngine);

	Vector3 center = polygonMesh.CalcVertexAverage();

	struct Vertex
	{
		Vector3 point;
		Vector3 normal;
	};

	std::vector<Vertex> vertexBufferData;
	const std::vector<Vector3>& vertexArray = polygonMesh.GetVertexArray();
	vertexBufferData.resize(polygonMesh.GetVertexArray().size());
	for (int i = 0; i < (int)vertexArray.size(); i++)
		vertexBufferData[i] = { vertexArray[i], (vertexArray[i] - center).Normalized() };

	VertexBuffer::VertexBufferSetupData vertexBufferSetupData;
	vertexBufferSetupData.bufferSetupData.type = VertexBuffer::Type::STATIC;
	vertexBufferSetupData.bufferSetupData.bufferData = (uint8_t*)vertexBufferData.data();
	vertexBufferSetupData.bufferSetupData.bufferSize = vertexBufferData.size() * sizeof(Vertex);
	vertexBufferSetupData.strideInBytes = sizeof(Vertex);
	vertexBufferSetupData.elementDescArray.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
	vertexBufferSetupData.elementDescArray.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
	if (!vertexBuffer->Setup(&vertexBufferSetupData))
	{
		THEBE_LOG("Failed to setup vertex buffer.");
		return false;
	}

	return true;
}