#include "TextureBuilder.h"
#include <wx/image.h>

TextureBuilder::TextureBuilder()
{
	this->compressTextures = true;
}

/*virtual*/ TextureBuilder::~TextureBuilder()
{
}

void TextureBuilder::ClearTextures()
{
	this->texturesToBuildMap.clear();
}

void TextureBuilder::AddTexture(const std::filesystem::path& inputTexturePath, const TextureBuildInfo& textureBuildInfo)
{
	this->texturesToBuildMap.insert(std::pair(inputTexturePath.lexically_normal(), textureBuildInfo));
}

bool TextureBuilder::GenerateTextures()
{
	THEBE_LOG("Processing %d textures...", this->texturesToBuildMap.size());
	for (auto pair : this->texturesToBuildMap)
	{
		const std::filesystem::path& inputTexturePath = pair.first;
		const TextureBuildInfo& textureBuildInfo = pair.second;
		THEBE_LOG("Processing texture: %s", inputTexturePath.string().c_str());

		Thebe::Reference<Thebe::TextureBuffer> outputTexture = this->GenerateTextureBuffer(inputTexturePath, textureBuildInfo);
		if (!outputTexture.Get())
		{
			THEBE_LOG("Failed to generate texture: %s", inputTexturePath.string().c_str());
			return false;
		}

		std::filesystem::path assetsFolder;
		if (!wxGetApp().GetGraphicsEngine()->GleanAssetsFolderFromPath(inputTexturePath, assetsFolder))
		{
			THEBE_LOG("Failed to glean assets folder from path: %s", inputTexturePath.string().c_str());
			return false;
		}

		std::filesystem::path outputTexturePath = (assetsFolder / this->GenerateTextureBufferPath(inputTexturePath)).lexically_normal();
		if (!wxGetApp().GetGraphicsEngine()->DumpEnginePartToFile(outputTexturePath, outputTexture, THEBE_DUMP_FLAG_CAN_OVERWRITE))
		{
			THEBE_LOG("Failed to dump texture: %s", outputTexturePath.string().c_str());
			return false;
		}
	}

	return true;
}

std::filesystem::path TextureBuilder::GenerateTextureBufferPath(const std::filesystem::path& inputTexturePath)
{
	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	std::filesystem::path relativeInputTexturePath = inputTexturePath;
	bool found = graphicsEngine->GetRelativeToAssetFolder(relativeInputTexturePath);
	THEBE_ASSERT(found);
	std::string inputTextureName(inputTexturePath.stem().string());
	std::string outputTextureName = inputTextureName + ".texture_buffer";
	outputTextureName = this->NoSpaces(outputTextureName);
	std::filesystem::path outputTexturePath = relativeInputTexturePath.parent_path() / outputTextureName;
	return outputTexturePath;
}

Thebe::Reference<Thebe::TextureBuffer> TextureBuilder::GenerateTextureBuffer(const std::filesystem::path& inputTexturePath, const TextureBuildInfo& buildInfo)
{
	Thebe::Reference<Thebe::TextureBuffer> outputTexture(new Thebe::TextureBuffer());
	outputTexture->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());
	outputTexture->SetName(inputTexturePath.stem().string());
	outputTexture->SetBufferType(Thebe::Buffer::STATIC);
	outputTexture->SetCompressed(this->compressTextures);

	wxImage inputImage;
	if (!inputImage.LoadFile(inputTexturePath.string().c_str()))
	{
		THEBE_LOG("Failed to load file: %s", inputTexturePath.string().c_str());
		return Thebe::Reference<Thebe::TextureBuffer>();
	}

	D3D12_RESOURCE_DESC& gpuBufferDesc = outputTexture->GetResourceDesc();
	gpuBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	gpuBufferDesc.Width = inputImage.GetWidth();
	gpuBufferDesc.Height = inputImage.GetHeight();
	gpuBufferDesc.Format = buildInfo.format;

	if (outputTexture->GetBytesPerPixel() == 0)
	{
		THEBE_LOG("Got zero bytes per pixel?!");
		return Thebe::Reference<Thebe::TextureBuffer>();
	}

	UINT64 outputBufferSize = 0;
	UINT numMips = 0;
	UINT64 mipWidth = (UINT)inputImage.GetWidth();
	UINT64 mipHeight = (UINT)inputImage.GetHeight();
	while (numMips < buildInfo.maxMips && mipWidth >= 1 && mipHeight >= 1)
	{
		numMips++;
		outputBufferSize += mipWidth * mipHeight * outputTexture->GetBytesPerPixel();
		if ((mipWidth & 0x1) != 0 || (mipHeight & 0x1) != 0)
			break;
		mipWidth >>= 1;
		mipHeight >>= 1;
	}

	gpuBufferDesc.MipLevels = numMips;

	UINT bytesPerInputColorPixel = 3;
	UINT bytesPerInputAlphaPixel = 1;

	std::vector<UINT8>& outputBuffer = outputTexture->GetOriginalBuffer();
	outputBuffer.resize(outputBufferSize);

	mipWidth = (UINT)inputImage.GetWidth();
	mipHeight = (UINT)inputImage.GetHeight();
	UINT64 mipImageOffset = 0;

	for (UINT mipNumber = 0; mipNumber < numMips; mipNumber++)
	{
		wxImage sourceImage;
		if (mipNumber > 0)
			sourceImage = inputImage.Scale((int)mipWidth, (int)mipHeight, wxIMAGE_QUALITY_HIGH);
		else
			sourceImage = inputImage;

		UINT8* destinationImageBuffer = &outputBuffer.data()[mipImageOffset];

		for (UINT64 i = 0; i < mipHeight; i++)
		{
			for (UINT64 j = 0; j < mipWidth; j++)
			{
				UINT pixelOffset = i * mipWidth + j;

				const unsigned char* inputColor = &sourceImage.GetData()[pixelOffset * bytesPerInputColorPixel];
				const unsigned char* inputAlpha = sourceImage.HasAlpha() ? &sourceImage.GetAlpha()[pixelOffset * bytesPerInputAlphaPixel] : nullptr;

				UINT8* outputPixel = &destinationImageBuffer[pixelOffset * outputTexture->GetBytesPerPixel()];

				switch (buildInfo.format)
				{
				case DXGI_FORMAT_R8G8B8A8_UNORM:
					outputPixel[0] = inputColor[0];
					outputPixel[1] = inputColor[1];
					outputPixel[2] = inputColor[2];
					outputPixel[3] = inputAlpha ? inputAlpha[0] : 0;
					break;
				case DXGI_FORMAT_R8G8_UNORM:
					outputPixel[0] = inputColor[0];
					outputPixel[1] = inputColor[1];
					break;
				case DXGI_FORMAT_R8_UNORM:
					outputPixel[0] = inputColor[0];
					break;
				case DXGI_FORMAT_A8_UNORM:
					outputPixel[0] = inputAlpha ? inputAlpha[0] : 0;
					break;
				default:
					THEBE_LOG("Pixel format (%d) not yet supported.", buildInfo.format);
					return Thebe::Reference<Thebe::TextureBuffer>();
				}
			}
		}

		mipImageOffset += mipWidth * mipHeight * outputTexture->GetBytesPerPixel();
		mipWidth >>= 1;
		mipHeight >>= 1;
	}

	return outputTexture;
}