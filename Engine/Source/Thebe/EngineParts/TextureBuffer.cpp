#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/UploadHeap.h"
#include "Thebe/Log.h"

using namespace Thebe;

TextureBuffer::TextureBuffer()
{
	this->gpuBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	this->offsetAlignmentRequirement = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
}

/*virtual*/ TextureBuffer::~TextureBuffer()
{
}

/*virtual*/ bool TextureBuffer::Setup()
{
	if (!Buffer::Setup())
		return false;

	return true;
}

/*virtual*/ void TextureBuffer::Shutdown()
{
	Buffer::Shutdown();
}

/*virtual*/ bool TextureBuffer::CreateResourceView(CD3DX12_CPU_DESCRIPTOR_HANDLE& handle, ID3D12Device* device)
{
	D3D12_RESOURCE_DESC resourceDesc = this->gpuBuffer->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = resourceDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(this->gpuBuffer.Get(), &srvDesc, handle);
	return true;
}

/*virtual*/ bool TextureBuffer::ValidateBufferDescription()
{
	if (this->gpuBufferDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		THEBE_LOG("Expected resource dimension to be TEXTURE2D.");
		return false;
	}
	
	UINT bytesPerPixel = this->GetBytesPerPixel();
	if (bytesPerPixel == 0)
		return false;

	if (this->gpuBufferDesc.Width * this->gpuBufferDesc.Height * bytesPerPixel != (UINT64)this->originalBuffer.size())
	{
		THEBE_LOG("Dimensions (%d x %d) and pixel depth are not consistent with the buffer size %ull.",
			this->gpuBufferDesc.Width,
			this->gpuBufferDesc.Height,
			UINT64(this->originalBuffer.size()));
		return false;
	}

	return true;
}

/*virtual*/ UINT64 TextureBuffer::GetUploadHeapAllocationSize(ID3D12Device* device)
{
	// Sometimes the description changes after resource creation, so get it.  Not sure it really matters.
	this->gpuBufferDesc = this->gpuBuffer->GetDesc();

	UINT64 numBytesToAllocateInUploadHeap = 0;
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, 1, 0, nullptr, nullptr, nullptr, &numBytesToAllocateInUploadHeap);
	return numBytesToAllocateInUploadHeap;
}

/*virtual*/ bool TextureBuffer::CopyDataToUploadHeap(UINT8* uploadBuffer, ID3D12Device* device)
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
	UINT64 rowSizeInBytes = 0;
	UINT numRows = 0;

	// We can't just copy it in willy-nilly.  We have to look at the footprint.
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, 1, 0, &layout, &numRows, &rowSizeInBytes, nullptr);

	if (numRows != this->gpuBufferDesc.Height)
	{
		THEBE_LOG("Mismatch between footprint rows (%d) and actual rows (%d).", numRows, this->gpuBufferDesc.Height);
		return false;
	}

	UINT64 bytesPerPixel = this->GetBytesPerPixel();
	if (rowSizeInBytes != this->gpuBufferDesc.Width * bytesPerPixel)
	{
		THEBE_LOG("Mismatch between footprint bytes per row (%d) and actual bytes per row (%d).",
			rowSizeInBytes, this->gpuBufferDesc.Width * bytesPerPixel);
		return false;
	}

	for (UINT i = 0; i < numRows; i++)
	{
		UINT8* sourceRow = &this->originalBuffer.data()[i * rowSizeInBytes];
		UINT8* destinationRow = &uploadBuffer[i * layout.Footprint.RowPitch];
		::memcpy(destinationRow, sourceRow, rowSizeInBytes);
	}

	return true;
}

UINT64 TextureBuffer::GetBytesPerPixel()
{
	if (this->gpuBufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
		return sizeof(UINT);

	THEBE_LOG("Pixel format %d not yet supported.", this->gpuBufferDesc.Format);
	return 0;
}

/*virtual*/ void TextureBuffer::CopyDataFromUploadHeapToDefaultHeap(UploadHeap* uploadHeap, ID3D12GraphicsCommandList* commandList, ID3D12Device* device)
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);

	D3D12_TEXTURE_COPY_LOCATION destinationCopyLocation{};
	destinationCopyLocation.pResource = this->gpuBuffer.Get();
	destinationCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	D3D12_TEXTURE_COPY_LOCATION sourceCopyLocation{};
	sourceCopyLocation.pResource = uploadHeap->GetUploadBuffer();
	sourceCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	sourceCopyLocation.PlacedFootprint.Offset = this->uploadBufferOffset + layout.Offset;
	sourceCopyLocation.PlacedFootprint.Footprint = layout.Footprint;

	commandList->CopyTextureRegion(&destinationCopyLocation, 0, 0, 0, &sourceCopyLocation, nullptr);
}

/*virtual*/ bool TextureBuffer::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!Buffer::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto widthValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("width"));
	auto heightValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("height"));
	if (!widthValue || !heightValue)
	{
		THEBE_LOG("Both height and width not present in JSON data.");
		return false;
	}

	auto pixelFormatValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("pixel_format"));
	if (!pixelFormatValue)
	{
		THEBE_LOG("No pixel format found.");
		return false;
	}

	D3D12_RESOURCE_DESC& resourceDesc = this->GetResourceDesc();
	resourceDesc.Width = (UINT64)widthValue->GetValue();
	resourceDesc.Height = (UINT64)heightValue->GetValue();
	resourceDesc.Format = (DXGI_FORMAT)pixelFormatValue->GetValue();

	return true;
}

/*virtual*/ bool TextureBuffer::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!Buffer::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	const D3D12_RESOURCE_DESC& resourceDesc = this->GetResourceDesc();
	rootValue->SetValue("width", new JsonInt(resourceDesc.Width));
	rootValue->SetValue("height", new JsonInt(resourceDesc.Height));
	rootValue->SetValue("pixel_format", new JsonInt(resourceDesc.Format));

	return true;
}