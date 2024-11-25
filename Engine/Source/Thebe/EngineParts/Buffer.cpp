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
	this->gpuBufferPtr = nullptr;
	this->type = Type::NONE;
	this->lastUpdateFrameCount = -1L;
}

/*virtual*/ Buffer::~Buffer()
{
	this->cpuBuffer.reset();
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
	if (this->slowMemBuffer.Get() || this->fastMemBuffer.Get())
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

	UINT32 bufferSize = (UINT32)this->originalBuffer.size();
	if (bufferSize == 0)
	{
		THEBE_LOG("Can't create buffer of size zero.");
		return false;
	}

	UINT32 i = (this->type == Type::DYNAMIC_N_BUFFER_METHOD) ? THEBE_NUM_SWAP_FRAMES : 1;
	auto slowMemBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize * i);
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
		this->cpuBuffer.reset(new UINT8[bufferSize]);
		::memcpy(this->cpuBuffer.get(), this->originalBuffer.data(), bufferSize);
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
		::memcpy(this->gpuBufferPtr, this->originalBuffer.data(), bufferSize);
		this->slowMemBuffer->Unmap(0, nullptr);
		this->gpuBufferPtr = nullptr;
	}

	if (this->type == Type::STATIC || this->type == Type::DYNAMIC_BARRIER_METHOD)
	{
		auto fastMemBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
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
		
		commandList->CopyBufferRegion(this->fastMemBuffer.Get(), 0, this->slowMemBuffer.Get(), 0, bufferSize);

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
	// Hmmm...I suppose we could issue GPU commands here to discard resources...

	this->slowMemBuffer = nullptr;
	this->fastMemBuffer = nullptr;
	this->gpuBufferPtr = nullptr;
	this->cpuBuffer.reset();
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

			UINT32 bufferSize = (UINT32)this->originalBuffer.size();
			commandList->CopyBufferRegion(this->fastMemBuffer.Get(), 0, this->slowMemBuffer.Get(), 0, bufferSize);

			barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->fastMemBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, this->resourceStateWhenRendering);
			commandList->ResourceBarrier(1, &barrier);

			return true;
		}
		case Type::DYNAMIC_N_BUFFER_METHOD:
		{
			if (!this->gpuBufferPtr || !this->cpuBuffer.get())
				return false;

			SwapChain* swapChain = graphicsEngine->GetSwapChain();
			if (!swapChain)
				return false;

			UINT32 bufferSize = (UINT32)this->originalBuffer.size();
			UINT32 i = swapChain->GetCurrentBackBufferIndex();
			UINT32 offset = i * bufferSize;
			::memcpy(&this->gpuBufferPtr[offset], this->cpuBuffer.get(), bufferSize);

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
		return this->cpuBuffer.get();
	}

	return nullptr;
}

UINT32 Buffer::GetBufferSize() const
{
	return (UINT32)this->originalBuffer.size();
}

void Buffer::SetBufferType(Type type)
{
	this->type = type;
}

Buffer::Type Buffer::GetBufferType() const
{
	return this->type;
}

/*virtual*/ bool Buffer::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath)
{
	using namespace ParseParty;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
	{
		THEBE_LOG("Expected root JSON value to be an object.");
		return false;
	}

	auto typeValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("type"));
	if (!typeValue)
	{
		THEBE_LOG("No type given.");
		return false;
	}

	this->type = (Type)typeValue->GetValue();

	auto sizeValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("size"));
	if (!sizeValue)
	{
		THEBE_LOG("No size given.");
		return false;
	}

	if (sizeValue->GetValue() <= 0)
	{
		THEBE_LOG("Invalid buffer size: %d", sizeValue->GetValue());
		return false;
	}

	UINT32 bufferSize = (UINT32)sizeValue->GetValue();
	this->originalBuffer.resize(bufferSize);

	auto floatArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("float_array"));
	auto intArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("int_array"));

	if (floatArrayValue)
	{
		if (floatArrayValue->GetSize() * sizeof(float) != bufferSize)
		{
			THEBE_LOG("Float array size (%d) is inconsistent with the given buffer size (%d).", floatArrayValue->GetSize(), bufferSize);
			return false;
		}

		auto floatBuffer = reinterpret_cast<float*>(this->originalBuffer.data());
		for (int i = 0; i < floatArrayValue->GetSize(); i++)
		{
			auto floatValue = dynamic_cast<const JsonFloat*>(floatArrayValue->GetValue(i));
			if (!floatValue)
			{
				THEBE_LOG("Expected float array entry to be a float value.");
				return false;
			}

			floatBuffer[i] = floatValue->GetValue();
		}
	}
	else if (intArrayValue)
	{
		if (intArrayValue->GetSize() * sizeof(int) != bufferSize)
		{
			THEBE_LOG("Integer array size (%d) is inconsistent with the given buffer size (%d).", intArrayValue->GetSize(), bufferSize);
			return false;
		}

		auto intBuffer = reinterpret_cast<int*>(this->originalBuffer.data());
		for (int i = 0; i < intArrayValue->GetSize(); i++)
		{
			auto intValue = dynamic_cast<const JsonInt*>(intArrayValue->GetValue(i));
			if (!intValue)
			{
				THEBE_LOG("Expected integer array entry to be an integer value.");
				return false;
			}

			intBuffer[i] = (int)intValue->GetValue();
		}
	}
	else
	{
		auto dataValue = dynamic_cast<const JsonString*>(rootValue->GetValue("data"));
		if (!dataValue)
		{
			THEBE_LOG("No data given.");
			return false;
		}

		std::filesystem::path bufferDataPath = dataValue->GetValue();
		if (!graphicsEngine->ResolvePath(bufferDataPath, GraphicsEngine::RELATIVE_TO_ASSET_FOLDER))
		{
			THEBE_LOG("Failed to resolve buffer data path: %s", bufferDataPath.c_str());
			return false;
		}

		std::ifstream fileStream;
		fileStream.open(bufferDataPath.c_str(), std::ios::in | std::ios::binary);
		if (!fileStream.is_open())
		{
			THEBE_LOG("Failed to open file: %s", bufferDataPath.c_str());
			return false;
		}

		fileStream.read((char*)this->originalBuffer.data(), bufferSize);
		fileStream.close();
	}

	return true;
}

/*virtual*/ bool Buffer::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const
{
	using namespace ParseParty;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	auto rootValue = new JsonObject();
	jsonValue.reset(rootValue);

	rootValue->SetValue("type", new JsonInt(this->type));
	rootValue->SetValue("size", new JsonInt(this->GetBufferSize()));

	// TODO: May want to compress the buffer.

	std::filesystem::path bufferDataPath = relativePath;
	bufferDataPath.replace_extension(bufferDataPath.extension().string() + "_data");

	rootValue->SetValue("data", new JsonString(bufferDataPath.string()));

	bufferDataPath = graphicsEngine->GetAssetFolder() / bufferDataPath;

	std::ofstream fileStream;
	fileStream.open(bufferDataPath.c_str(), std::ios::out | std::ios::binary);
	if (!fileStream.is_open())
	{
		THEBE_LOG("Failed to open file %s for writing binary.", bufferDataPath.c_str());
		return false;
	}

	fileStream.write((const char*)this->originalBuffer.data(), this->originalBuffer.size());
	fileStream.close();

	return true;
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

	vertexBuffer.Set(new VertexBuffer());
	vertexBuffer->SetGraphicsEngine(graphicsEngine);
	vertexBuffer->SetBufferType(Buffer::STATIC);
	vertexBuffer->SetStride(6 * sizeof(float));
	std::vector<D3D12_INPUT_ELEMENT_DESC>& elementDescArray = vertexBuffer->GetElementDescArray();
	elementDescArray.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
	elementDescArray.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

	Vector3 center = polygonMesh.CalcVertexAverage();
	std::vector<UINT8>& originalVertexBuffer = vertexBuffer->GetOriginalBuffer();
	const std::vector<Vector3>& vertexArray = polygonMesh.GetVertexArray();
	originalVertexBuffer.resize(vertexArray.size() * 6 * sizeof(float));
	auto vertexBufferData = (float*)originalVertexBuffer.data();
	for (int i = 0; i < (int)vertexArray.size(); i++)
	{
		Vector3 point = vertexArray[i];
		Vector3 normal = (point - center).Normalized();
		float* vertex = &vertexBufferData[i * 6];
		*vertex++ = (float)point.x;
		*vertex++ = (float)point.y;
		*vertex++ = (float)point.z;
		*vertex++ = (float)normal.x;
		*vertex++ = (float)normal.y;
		*vertex++ = (float)normal.z;
	}

	if (!vertexBuffer->Setup())
	{
		THEBE_LOG("Failed to setup vertex buffer.");
		return false;
	}

	return true;
}