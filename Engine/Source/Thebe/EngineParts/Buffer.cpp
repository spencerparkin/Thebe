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
	this->type = Type::NONE;
	this->lastUpdateFrameCount = -1L;
}

/*virtual*/ Buffer::~Buffer()
{
	delete[] this->cpuBufferPtr;
}

std::vector<UINT8>& Buffer::GetOriginalBuffer()
{
	return this->originalBuffer;
}

const std::vector<UINT8>& Buffer::GetOriginalBuffer() const
{
	return this->originalBuffer;
}

/*virtual*/ bool Buffer::Setup()
{
	if (this->bufferSize > 0)
	{
		THEBE_LOG("Buffer already setup.");
		return false;
	}

	if (this->type == Type::NONE)
	{
		THEBE_LOG("Buffer type not configured.");
		return false;
	}

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	if (!device)
		return false;

	this->bufferSize = (UINT32)this->originalBuffer.size();
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
		::memcpy(this->cpuBufferPtr, this->originalBuffer.data(), this->bufferSize);
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
		::memcpy(this->gpuBufferPtr, this->originalBuffer.data(), this->bufferSize);
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

bool Buffer::UpdateIfNecessary(ID3D12GraphicsCommandList* commandList)
{
	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UINT64 frameCount = graphicsEngine->GetFrameCount();
	if (frameCount == this->lastUpdateFrameCount)
		return true;

	this->lastUpdateFrameCount = frameCount;

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

void Buffer::SetBufferType(Type type)
{
	this->type = type;
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
	indexBuffer->SetBufferType(Buffer::STATIC);
	indexBuffer->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	indexBuffer->SetFormat(DXGI_FORMAT_R16_UINT);

	std::vector<UINT8>& originalIndexBuffer = indexBuffer->GetOriginalBuffer();
	originalIndexBuffer.resize(polygonArray.size() * 3 * sizeof(uint16_t));
	auto indexBufferData = (uint16_t*)originalIndexBuffer.data();;
	int i = 0;
	for (const auto& polygon : polygonArray)
		for (int j = 0; j < (int)polygon.vertexArray.size(); j++)
			indexBufferData[i++] = (uint16_t)polygon.vertexArray[j];

	if (!indexBuffer->Setup())
	{
		THEBE_LOG("Failed to setup index buffer.");
		return false;
	}

	struct Vertex
	{
		Vector3 point;
		Vector3 normal;
	};

	vertexBuffer.Set(new VertexBuffer());
	vertexBuffer->SetGraphicsEngine(graphicsEngine);
	vertexBuffer->SetBufferType(Buffer::STATIC);
	vertexBuffer->SetStride(sizeof(Vertex));
	std::vector<D3D12_INPUT_ELEMENT_DESC>& elementDescArray = vertexBuffer->GetElementDescArray();
	elementDescArray.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
	elementDescArray.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

	Vector3 center = polygonMesh.CalcVertexAverage();
	std::vector<UINT8>& originalVertexBuffer = vertexBuffer->GetOriginalBuffer();
	const std::vector<Vector3>& vertexArray = polygonMesh.GetVertexArray();
	originalVertexBuffer.resize(vertexArray.size() * sizeof(Vertex));
	auto vertexBufferData = (Vertex*)originalVertexBuffer.data();
	for (int i = 0; i < (int)vertexArray.size(); i++)
		vertexBufferData[i] = { vertexArray[i], (vertexArray[i] - center).Normalized() };

	if (!vertexBuffer->Setup())
	{
		THEBE_LOG("Failed to setup vertex buffer.");
		return false;
	}

	return true;
}