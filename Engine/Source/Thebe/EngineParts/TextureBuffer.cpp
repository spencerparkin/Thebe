#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/UploadHeap.h"
#include "Thebe/Log.h"

using namespace Thebe;

TextureBuffer::TextureBuffer()
{
	this->gpuBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	this->offsetAlignmentRequirement = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
	this->sizeAlignmentRequirement = 1;
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
	srvDesc.Texture2D.MipLevels = this->gpuBufferDesc.MipLevels;
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

	UINT64 totalSize = 0;
	UINT64 mipWidth = this->gpuBufferDesc.Width;
	UINT64 mipHeight = this->gpuBufferDesc.Height;
	for (UINT i = 0; i < this->gpuBufferDesc.MipLevels; i++)
	{
		totalSize += mipWidth * mipHeight * bytesPerPixel;
		mipWidth >>= 1;
		mipHeight >>= 1;
	}

	if (totalSize != (UINT64)this->originalBuffer.size())
	{
		THEBE_LOG("Size of original buffer (%ull) inconsistent with calculated size (%ull) according to MIP count, etc.", UINT64(this->originalBuffer.size()), totalSize);
		return false;
	}

	return true;
}

/*virtual*/ UINT64 TextureBuffer::GetUploadHeapAllocationSize(ID3D12Device* device)
{
	// Sometimes the description changes after resource creation, so get it.  Not sure it really matters.
	this->gpuBufferDesc = this->gpuBuffer->GetDesc();

	UINT64 numBytesToAllocateInUploadHeap = 0;
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, this->gpuBufferDesc.MipLevels, 0, nullptr, nullptr, nullptr, &numBytesToAllocateInUploadHeap);
	return numBytesToAllocateInUploadHeap;
}

/*virtual*/ bool TextureBuffer::CopyDataToUploadHeap(UINT8* uploadBuffer, ID3D12Device* device)
{
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutArray;
	std::vector<UINT64> rowSizeArray;
	std::vector<UINT> numRowsArray;

	layoutArray.resize(this->gpuBufferDesc.MipLevels);
	rowSizeArray.resize(this->gpuBufferDesc.MipLevels);
	numRowsArray.resize(this->gpuBufferDesc.MipLevels);

	// We can't just copy it in willy-nilly.  We have to look at the footprint.
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, this->gpuBufferDesc.MipLevels, 0, layoutArray.data(), numRowsArray.data(), rowSizeArray.data(), nullptr);

	UINT64 bytesPerPixel = this->GetBytesPerPixel();
	UINT64 mipWidth = this->gpuBufferDesc.Width;
	UINT64 mipHeight = this->gpuBufferDesc.Height;
	UINT64 sourceOffset = 0;

	for (UINT i = 0; i < this->gpuBufferDesc.MipLevels; i++)
	{
		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layoutArray[i];
		UINT64 rowSizeInBytes = rowSizeArray[i];
		UINT numRows = numRowsArray[i];
		
		if (rowSizeInBytes != mipWidth * bytesPerPixel)
			return false;

		for (UINT j = 0; j < numRows; j++)
		{
			UINT8* sourceRow = &this->originalBuffer.data()[sourceOffset + j * rowSizeInBytes];
			UINT8* destinationRow = &uploadBuffer[layout.Offset + j * layout.Footprint.RowPitch];
			::memcpy(destinationRow, sourceRow, rowSizeInBytes);
		}

		sourceOffset += mipWidth * mipHeight * bytesPerPixel;
		mipWidth >>= 1;
		mipHeight >>= 1;
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
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutArray;
	layoutArray.resize(this->gpuBufferDesc.MipLevels);
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, this->gpuBufferDesc.MipLevels, 0, layoutArray.data(), nullptr, nullptr, nullptr);

	for (UINT i = 0; i < this->gpuBufferDesc.MipLevels; i++)
	{
		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layoutArray[i];

		D3D12_TEXTURE_COPY_LOCATION destinationCopyLocation{};
		destinationCopyLocation.pResource = this->gpuBuffer.Get();
		destinationCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destinationCopyLocation.SubresourceIndex = i;

		D3D12_TEXTURE_COPY_LOCATION sourceCopyLocation{};
		sourceCopyLocation.pResource = uploadHeap->GetUploadBuffer();
		sourceCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		sourceCopyLocation.PlacedFootprint.Offset = this->uploadBufferOffset + layout.Offset;
		sourceCopyLocation.PlacedFootprint.Footprint = layout.Footprint;

		commandList->CopyTextureRegion(&destinationCopyLocation, 0, 0, 0, &sourceCopyLocation, nullptr);
	}
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

	auto numMipsValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("num_mips"));
	if (!numMipsValue)
	{
		THEBE_LOG("No num-MIPs value found.");
		return false;
	}

	D3D12_RESOURCE_DESC& resourceDesc = this->GetResourceDesc();
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = (UINT64)widthValue->GetValue();
	resourceDesc.Height = (UINT64)heightValue->GetValue();
	resourceDesc.Format = (DXGI_FORMAT)pixelFormatValue->GetValue();
	resourceDesc.MipLevels = (UINT16)numMipsValue->GetValue();

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
	rootValue->SetValue("num_mips", new JsonInt(resourceDesc.MipLevels));

	return true;
}