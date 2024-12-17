#include "Thebe/EngineParts/CubeMapBuffer.h"
#include "Thebe/EngineParts/UploadHeap.h"
#include "Thebe/Log.h"

using namespace Thebe;

CubeMapBuffer::CubeMapBuffer()
{
	this->gpuBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	this->offsetAlignmentRequirement = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
	this->sizeAlignmentRequirement = 1;
}

/*virtual*/ CubeMapBuffer::~CubeMapBuffer()
{
}

/*virtual*/ bool CubeMapBuffer::Setup()
{
	if (!Buffer::Setup())
		return false;

	return true;
}

/*virtual*/ void CubeMapBuffer::Shutdown()
{
	Buffer::Shutdown();
}

/*virtual*/ bool CubeMapBuffer::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
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

	auto arraySizeValue = dynamic_cast<const JsonInt*>(rootValue->GetValue("array_size"));
	if (!arraySizeValue)
	{
		THEBE_LOG("No array size value found.");
		return false;
	}

	D3D12_RESOURCE_DESC& resourceDesc = this->GetResourceDesc();
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = (UINT64)widthValue->GetValue();
	resourceDesc.Height = (UINT64)heightValue->GetValue();
	resourceDesc.Format = (DXGI_FORMAT)pixelFormatValue->GetValue();
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = (UINT16)arraySizeValue->GetValue();

	return true;
}

/*virtual*/ bool CubeMapBuffer::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
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
	rootValue->SetValue("array_size", new JsonInt(resourceDesc.DepthOrArraySize));

	return true;
}

/*virtual*/ bool CubeMapBuffer::CreateResourceView(CD3DX12_CPU_DESCRIPTOR_HANDLE& handle, ID3D12Device* device)
{
	D3D12_RESOURCE_DESC resourceDesc = this->gpuBuffer->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = resourceDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = 1;
	device->CreateShaderResourceView(this->gpuBuffer.Get(), &srvDesc, handle);
	return true;
}

/*virtual*/ bool CubeMapBuffer::ValidateBufferDescription()
{
	if (this->gpuBufferDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		THEBE_LOG("Expected resource dimension to be TEXTURE2D");
		return false;
	}

	UINT bytesPerPixel = this->GetBytesPerPixel();
	if (bytesPerPixel == 0)
		return false;

	UINT64 totalSize = 0;
	for (UINT i = 0; i < this->gpuBufferDesc.DepthOrArraySize; i++)
		totalSize += this->gpuBufferDesc.Width * this->gpuBufferDesc.Height * bytesPerPixel;

	if (totalSize != (UINT64)this->originalBuffer.size())
	{
		THEBE_LOG("Size of original buffer (%ull) inconsistent with calculated size (%ull) according to descript.", UINT64(this->originalBuffer.size()), totalSize);
		return false;
	}

	return true;
}

/*virtual*/ UINT64 CubeMapBuffer::GetUploadHeapAllocationSize(ID3D12Device* device)
{
	UINT64 numBytesToAllocateInUploadHeap = 0;
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, this->gpuBufferDesc.DepthOrArraySize, 0, nullptr, nullptr, nullptr, &numBytesToAllocateInUploadHeap);
	return numBytesToAllocateInUploadHeap;
}

/*virtual*/ bool CubeMapBuffer::CopyDataToUploadHeap(UINT8* uploadBuffer, ID3D12Device* device)
{
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutArray;
	std::vector<UINT64> rowSizeArray;
	std::vector<UINT> numRowsArray;

	layoutArray.resize(this->gpuBufferDesc.DepthOrArraySize);
	rowSizeArray.resize(this->gpuBufferDesc.DepthOrArraySize);
	numRowsArray.resize(this->gpuBufferDesc.DepthOrArraySize);

	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, this->gpuBufferDesc.DepthOrArraySize, 0, layoutArray.data(), numRowsArray.data(), rowSizeArray.data(), nullptr);

	UINT64 bytesPerPixel = this->GetBytesPerPixel();
	UINT64 sourceOffset = 0;

	for (UINT i = 0; i < this->gpuBufferDesc.DepthOrArraySize; i++)
	{
		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layoutArray[i];
		UINT64 rowSizeInBytes = rowSizeArray[i];
		UINT numRows = numRowsArray[i];

		if (rowSizeInBytes != this->gpuBufferDesc.Width * bytesPerPixel)
			return false;

		for (UINT j = 0; j < numRows; j++)
		{
			UINT8* sourceRow = &this->originalBuffer.data()[sourceOffset + j * rowSizeInBytes];
			UINT8* destinationRow = &uploadBuffer[layout.Offset + j * layout.Footprint.RowPitch];
			::memcpy(destinationRow, sourceRow, rowSizeInBytes);
		}

		sourceOffset += this->gpuBufferDesc.Width * this->gpuBufferDesc.Height * bytesPerPixel;
	}

	return true;
}

/*virtual*/ void CubeMapBuffer::CopyDataFromUploadHeapToDefaultHeap(UploadHeap* uploadHeap, ID3D12GraphicsCommandList* commandList, ID3D12Device* device)
{
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutArray;
	layoutArray.resize(this->gpuBufferDesc.DepthOrArraySize);
	device->GetCopyableFootprints(&this->gpuBufferDesc, 0, this->gpuBufferDesc.DepthOrArraySize, 0, layoutArray.data(), nullptr, nullptr, nullptr);

	for (UINT i = 0; i < this->gpuBufferDesc.DepthOrArraySize; i++)
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