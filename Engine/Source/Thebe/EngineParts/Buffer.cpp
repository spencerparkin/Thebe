#include "Thebe/EngineParts/Buffer.h"
#include "Thebe/EngineParts/SwapChain.h"
#include "Thebe/EngineParts/CommandAllocator.h"
#include "Thebe/EngineParts/IndexBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/UploadHeap.h"
#include "Thebe/Utilities/CompressionHelper.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"
#include "Thebe/Math/PolygonMesh.h"
#include "Thebe/Math/Vector2.h"
#include <d3dx12.h>
#include <locale>
#include <codecvt>

using namespace Thebe;

Buffer::Buffer()
{
	this->resourceStateWhenRendering = D3D12_RESOURCE_STATE_COMMON;
	this->type = Type::NONE;
	this->lastUpdateFrameCount = -1L;
	this->uploadBufferOffset = 0L;
	this->compressed = false;

	// These alignment conventions are not well documented, but apparently well understood.
	this->offsetAlignmentRequirement = 256;
	this->sizeAlignmentRequirement = 256;

	this->gpuBufferDesc.MipLevels = 1;
	this->gpuBufferDesc.Alignment = 0;
	this->gpuBufferDesc.Width = 0;
	this->gpuBufferDesc.Height = 1;
	this->gpuBufferDesc.DepthOrArraySize = 1;
	this->gpuBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	this->gpuBufferDesc.SampleDesc.Count = 1;
	this->gpuBufferDesc.SampleDesc.Quality = 0;
	this->gpuBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	this->gpuBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	this->gpuBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
}

/*virtual*/ Buffer::~Buffer()
{
}

void Buffer::SetCompressed(bool compressed)
{
	this->compressed = compressed;
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
	if (this->gpuBuffer.Get())
	{
		THEBE_LOG("Buffer already setup.");
		return false;
	}

	if (this->type == Type::NONE)
	{
		THEBE_LOG("Buffer type not configured.");
		return false;
	}

	UINT64 bufferSize = (UINT64)this->originalBuffer.size();
	if (bufferSize == 0)
	{
		THEBE_LOG("Can't create buffer of size zero.");
		return false;
	}

	if (bufferSize % this->sizeAlignmentRequirement != 0)
	{
		THEBE_LOG("Buffer sizes should be multiples of %ull in this case.", this->sizeAlignmentRequirement);
		return false;
	}

	if (this->gpuBufferDesc.Dimension == D3D12_RESOURCE_DIMENSION_UNKNOWN)
	{
		THEBE_LOG("Buffer resource dimension not specified.");
		return false;
	}

	if (!this->ValidateBufferDescription())
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UploadHeap* uploadHeap = graphicsEngine->GetUploadHeap();
	if (!uploadHeap)
		return false;

	ID3D12Device* device = graphicsEngine->GetDevice();
	if (!device)
		return false;

	CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT result = device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&this->gpuBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&this->gpuBuffer));
	if (FAILED(result))
	{
		THEBE_LOG("Failed to create GPU buffer.  Error: 0x%08x", result);
		return false;
	}

	if (this->name.length() > 0)
	{
		std::wstring nameWide = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->name);
		result = this->gpuBuffer->SetName(nameWide.c_str());
		if (FAILED(result))
		{
			THEBE_LOG("Failed to name buffer.  Error: 0x%08x", result);
			return false;
		}
	}

	UINT64 numBytesToAllocateInUploadHeap = this->GetUploadHeapAllocationSize(device);
	if (numBytesToAllocateInUploadHeap == 0)
	{
		THEBE_LOG("Failed to determine the amount of memory needed in the upload heap.");
		return false;
	}

	if (!uploadHeap->Allocate(numBytesToAllocateInUploadHeap, this->offsetAlignmentRequirement, this->uploadBufferOffset))
	{
		THEBE_LOG("Failed to allocate %ull bytes in upload heap.", bufferSize);
		return false;
	}

	UINT8* uploadBuffer = uploadHeap->GetAllocationPtr(this->uploadBufferOffset);
	THEBE_ASSERT_FATAL(uploadBuffer != nullptr);

	if (!this->CopyDataToUploadHeap(uploadBuffer, device))
		return false;

	CommandAllocator* commandAllocator = graphicsEngine->GetCommandAllocator();
	if (!commandAllocator)
		return false;

	if(!commandAllocator->BeginRecordingCommandList())
		return false;

	ID3D12GraphicsCommandList* commandList = commandAllocator->GetCommandList();
	if (!commandList)
		return false;
	
	this->CopyDataFromUploadHeapToDefaultHeap(uploadHeap, commandList, device);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->gpuBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, this->resourceStateWhenRendering);
	commandList->ResourceBarrier(1, &barrier);

	if (!commandAllocator->EndRecordingCommandList())
		return false;

	if (this->type == Type::STATIC)
	{
		uploadHeap->Deallocate(this->uploadBufferOffset);
		this->uploadBufferOffset = 0L;
	}

	return true;
}

/*virtual*/ bool Buffer::CreateResourceView(CD3DX12_CPU_DESCRIPTOR_HANDLE& handle, ID3D12Device* device)
{
	return false;
}

/*virtual*/ bool Buffer::ValidateBufferDescription()
{
	if (this->gpuBufferDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		THEBE_LOG("Expected resource dimension to be BUFFER.");
		return false;
	}

	if (this->gpuBufferDesc.Width != (UINT64)this->originalBuffer.size() || this->gpuBufferDesc.Height != 1)
	{
		THEBE_LOG("Buffer description inconsistent with buffer size.");
		THEBE_LOG("Buffer width is set to %ull, but buffer size is set to %ull.", this->gpuBufferDesc.Width, UINT64(this->originalBuffer.size()));
		return false;
	}
	
	return true;
}

/*virtual*/ UINT64 Buffer::GetUploadHeapAllocationSize(ID3D12Device* device)
{
	return (UINT64)this->originalBuffer.size();
}

/*virtual*/ bool Buffer::CopyDataToUploadHeap(UINT8* uploadBuffer, ID3D12Device* device)
{
	UINT64 bufferSize = (UINT64)this->originalBuffer.size();
	::memcpy(uploadBuffer, this->originalBuffer.data(), bufferSize);
	return true;
}

/*virtual*/ void Buffer::CopyDataFromUploadHeapToDefaultHeap(UploadHeap* uploadHeap, ID3D12GraphicsCommandList* commandList, ID3D12Device* device)
{
	UINT64 bufferSize = (UINT64)this->originalBuffer.size();
	commandList->CopyBufferRegion(this->gpuBuffer.Get(), 0, uploadHeap->GetUploadBuffer(), this->uploadBufferOffset, bufferSize);
}

/*virtual*/ void Buffer::Shutdown()
{
	// Hmmm...I suppose we could issue GPU commands here to discard resources...
	// See: commandList->DiscardResource(...)

	if (this->type == Type::DYNAMIC)
	{
		Reference<GraphicsEngine> graphicsEngine;
		if (this->GetGraphicsEngine(graphicsEngine))
		{
			UploadHeap* uploadHeap = graphicsEngine->GetUploadHeap();
			uploadHeap->Deallocate(this->uploadBufferOffset);
		}
	}

	this->gpuBuffer = nullptr;
	this->type = Type::NONE;
}

bool Buffer::UpdateIfNecessary(ID3D12GraphicsCommandList* commandList)
{
	// It's never necessary to update a static buffer, but there's no penalty for calling this function.
	if (this->type != Type::DYNAMIC)
		return true;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	UploadHeap* uploadHeap = graphicsEngine->GetUploadHeap();

	// We're potentially called multiple times in the course of building up a single frame, but we need only update once per frame.
	UINT64 frameCount = graphicsEngine->GetFrameCount();
	if (frameCount == this->lastUpdateFrameCount)
		return true;

	this->lastUpdateFrameCount = frameCount;

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->gpuBuffer.Get(), this->resourceStateWhenRendering, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->ResourceBarrier(1, &barrier);

	this->CopyDataFromUploadHeapToDefaultHeap(uploadHeap, commandList, graphicsEngine->GetDevice());

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(this->gpuBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, this->resourceStateWhenRendering);
	commandList->ResourceBarrier(1, &barrier);

	return true;
}

UINT8* Buffer::GetBufferPtr()
{
	if (this->type != Type::DYNAMIC)
		return nullptr;

	Reference<GraphicsEngine> graphicsEngine;
	this->GetGraphicsEngine(graphicsEngine);
	UploadHeap* uploadHeap = graphicsEngine->GetUploadHeap();
	return uploadHeap->GetAllocationPtr(this->uploadBufferOffset);
}

const UINT8* Buffer::GetBufferPtr() const
{
	return const_cast<Buffer*>(this)->GetBufferPtr();
}

UINT64 Buffer::GetBufferSize() const
{
	return (UINT64)this->originalBuffer.size();
}

void Buffer::SetBufferType(Type type)
{
	this->type = type;
}

Buffer::Type Buffer::GetBufferType() const
{
	return this->type;
}

D3D12_RESOURCE_DESC& Buffer::GetResourceDesc()
{
	return this->gpuBufferDesc;
}

const D3D12_RESOURCE_DESC& Buffer::GetResourceDesc() const
{
	return this->gpuBufferDesc;
}

/*virtual*/ bool Buffer::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!EnginePart::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

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

	this->gpuBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	this->gpuBufferDesc.Width = this->GetBufferSize();
	this->gpuBufferDesc.Height = 1;

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

			floatBuffer[i] = (float)floatValue->GetValue();
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
			THEBE_LOG("Failed to resolve buffer data path: %s", bufferDataPath.string().c_str());
			return false;
		}

		std::ifstream fileStream;
		fileStream.open(bufferDataPath.c_str(), std::ios::in | std::ios::binary);
		if (!fileStream.is_open())
		{
			THEBE_LOG("Failed to open file: %s", bufferDataPath.string().c_str());
			return false;
		}

		auto compressedValue = dynamic_cast<const JsonBool*>(rootValue->GetValue("compressed"));
		if(!compressedValue || !compressedValue->GetValue())
			fileStream.read((char*)this->originalBuffer.data(), bufferSize);
		else
		{
			auto compressedSizeValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("compressed_size"));
			if (!compressedSizeValue)
			{
				THEBE_LOG("Compressed size was not stored in the JSON.");
				return false;
			}

			UINT64 compressedBufferSize = (UINT64)compressedSizeValue->GetValue();
			std::vector<UINT8> compressedBuffer;
			compressedBuffer.resize(compressedBufferSize);
			fileStream.read((char*)compressedBuffer.data(), compressedBufferSize);
			this->originalBuffer.resize(bufferSize);
			ZLibCompressionHelper compressionHelper;
			if (!compressionHelper.Decompress(compressedBuffer, this->originalBuffer))
				return false;

			if (this->originalBuffer.size() != bufferSize)
			{
				THEBE_LOG("Decompressed buffer size (%ull) does not match decompressed buffer size (%ull).", UINT64(this->originalBuffer.size()), bufferSize);
				return false;
			}
		}

		fileStream.close();
	}

	return true;
}

/*virtual*/ bool Buffer::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!EnginePart::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	Reference<GraphicsEngine> graphicsEngine;
	if (!this->GetGraphicsEngine(graphicsEngine))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	rootValue->SetValue("type", new JsonInt(this->type));
	rootValue->SetValue("size", new JsonInt(this->GetBufferSize()));

	std::filesystem::path bufferDataPath = assetPath;
	bufferDataPath.replace_extension(bufferDataPath.extension().string() + "_data");

	std::ofstream fileStream;
	fileStream.open(bufferDataPath.c_str(), std::ios::out | std::ios::binary);
	if (!fileStream.is_open())
	{
		THEBE_LOG("Failed to open file %s for writing binary.", bufferDataPath.string().c_str());
		return false;
	}

	rootValue->SetValue("compressed", new JsonBool(this->compressed));

	if(!this->compressed)
		fileStream.write((const char*)this->originalBuffer.data(), this->originalBuffer.size());
	else
	{
		std::vector<UINT8> compressedBuffer;
		ZLibCompressionHelper compressionHelper;
		if (!compressionHelper.Compress(this->originalBuffer, compressedBuffer))
			return false;

		fileStream.write((const char*)compressedBuffer.data(), compressedBuffer.size());
		rootValue->SetValue("compressed_size", new JsonInt(compressedBuffer.size()));
	}

	fileStream.close();

	if (!graphicsEngine->GetRelativeToAssetFolder(bufferDataPath))
		return false;

	rootValue->SetValue("data", new JsonString(bufferDataPath.string()));

	return true;
}

UINT64 Buffer::GetBytesPerPixel()
{
	switch (this->gpuBufferDesc.Format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case DXGI_FORMAT_R8G8_UNORM:
		return 2;
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
		return 1;
	}

	THEBE_LOG("Pixel format %d not yet supported.", this->gpuBufferDesc.Format);
	return 0;
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

	D3D12_RESOURCE_DESC& indexBufferDesc = indexBuffer->GetResourceDesc();
	indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	indexBufferDesc.Width = originalIndexBuffer.size();

	if (!indexBuffer->Setup())
	{
		THEBE_LOG("Failed to setup index buffer.");
		return false;
	}

	int floatsPerVertex = 8;
	vertexBuffer.Set(new VertexBuffer());
	vertexBuffer->SetGraphicsEngine(graphicsEngine);
	vertexBuffer->SetBufferType(Buffer::STATIC);
	vertexBuffer->SetStride(floatsPerVertex * sizeof(float));
	std::vector<D3D12_INPUT_ELEMENT_DESC>& elementDescArray = vertexBuffer->GetElementDescArray();
	elementDescArray.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
	elementDescArray.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
	elementDescArray.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

	Vector3 center = polygonMesh.CalcVertexAverage();
	std::vector<UINT8>& originalVertexBuffer = vertexBuffer->GetOriginalBuffer();
	const std::vector<Vector3>& vertexArray = polygonMesh.GetVertexArray();
	originalVertexBuffer.resize(vertexArray.size() * floatsPerVertex * sizeof(float));
	auto vertexBufferData = (float*)originalVertexBuffer.data();
	for (int i = 0; i < (int)vertexArray.size(); i++)
	{
		Vector3 point = vertexArray[i];
		Vector3 normal = (point - center).Normalized();
		Vector2 texCoord((point.x + 1.0) / 2.0, (point.y + 1.0) / 2.0);
		float* vertex = &vertexBufferData[i * floatsPerVertex];
		*vertex++ = (float)point.x;
		*vertex++ = (float)point.y;
		*vertex++ = (float)point.z;
		*vertex++ = (float)normal.x;
		*vertex++ = (float)normal.y;
		*vertex++ = (float)normal.z;
		*vertex++ = (float)texCoord.x;
		*vertex++ = (float)texCoord.y;
	}

	D3D12_RESOURCE_DESC& vertexBufferDesc = vertexBuffer->GetResourceDesc();
	vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferDesc.Width = originalVertexBuffer.size();

	if (!vertexBuffer->Setup())
	{
		THEBE_LOG("Failed to setup vertex buffer.");
		return false;
	}

	return true;
}

/*static*/ bool Buffer::GenerateCheckerboardTextureBuffer(
	UINT width,
	UINT height,
	UINT checkerSize,
	GraphicsEngine* graphicsEngine,
	Reference<TextureBuffer>& textureBuffer)
{
	textureBuffer.Set(new TextureBuffer());
	textureBuffer->SetGraphicsEngine(graphicsEngine);
	textureBuffer->SetBufferType(Buffer::STATIC);

	D3D12_RESOURCE_DESC& textureBufferDesc = textureBuffer->GetResourceDesc();
	textureBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureBufferDesc.Width = width;
	textureBufferDesc.Height = height;
	textureBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	UINT bytesPerPixel = 4;
	std::vector<UINT8>& originalTextureBuffer = textureBuffer->GetOriginalBuffer();
	originalTextureBuffer.resize(width * height * bytesPerPixel);

	for (UINT i = 0; i < width; i++)
	{
		for (UINT j = 0; j < height; j++)
		{
			UINT k = j * width + i;
			UINT8* pixel = &originalTextureBuffer.data()[k * bytesPerPixel];
			UINT x = i / checkerSize;
			UINT y = j / checkerSize;
			UINT component = ((x + y) % 2 == 0) ? 255 : 0;
			pixel[0] = component;
			pixel[1] = component;
			pixel[2] = component;
			pixel[3] = 255;
		}
	}

	if (!textureBuffer->Setup())
	{
		THEBE_LOG("Failed to setup texture buffer.");
		return false;
	}

	return true;
}