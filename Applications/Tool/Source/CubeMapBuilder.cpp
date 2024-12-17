#include "CubeMapBuilder.h"
#include "Thebe/EngineParts/CubeMapBuffer.h"
#include "Thebe/Log.h"
#include <wx/image.h>

CubeMapBuilder::CubeMapBuilder()
{
	this->compressTextures = true;
}

/*virtual*/ CubeMapBuilder::~CubeMapBuilder()
{
}

bool CubeMapBuilder::BuildCubeMap(const wxArrayString& inputCubeMapTexturesArray)
{
	if (inputCubeMapTexturesArray.size() == 0)
	{
		// TODO: Can we call on the graphics engine to render the sides of the cube map?
		//       Of course, to do this, we'd have to be able to read pixels back from the GPU into CPU memory.
	}

	if (inputCubeMapTexturesArray.size() != 6)
	{
		THEBE_LOG("A cube map needs 6 textures.");
		return false;
	}

	std::filesystem::path outputFolderPath = std::filesystem::path((const char*)inputCubeMapTexturesArray[0].c_str()).parent_path();
	std::string cubeMapName = outputFolderPath.stem().string();
	Thebe::Reference<Thebe::CubeMapBuffer> outputCubeMap(new Thebe::CubeMapBuffer());
	outputCubeMap->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());
	outputCubeMap->SetName(cubeMapName);
	outputCubeMap->SetCompressed(this->compressTextures);

	wxImage inputImageArray[6];
	for (int i = 0; i < 6; i++)
	{
		if (!inputImageArray[i].LoadFile(inputCubeMapTexturesArray[i]))
		{
			THEBE_LOG("Failed to load file: %s", inputCubeMapTexturesArray[i].c_str());
			return false;
		}

		if (i > 0)
		{
			if (inputImageArray[i].GetWidth() != inputImageArray[0].GetWidth() ||
				inputImageArray[i].GetHeight() != inputImageArray[0].GetHeight())
			{
				THEBE_LOG("All input textures must be the same size.");
				return false;
			}
		}
	}

	D3D12_RESOURCE_DESC& gpuBufferDesc = outputCubeMap->GetResourceDesc();
	gpuBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	gpuBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpuBufferDesc.Width = inputImageArray[0].GetWidth();
	gpuBufferDesc.Height = inputImageArray[0].GetHeight();
	gpuBufferDesc.DepthOrArraySize = 6;
	gpuBufferDesc.MipLevels = 1;

	UINT bytesPerInputColorPixel = 3;
	UINT bytesPerInputAlphaPixel = 1;

	std::vector<UINT8>& outputBuffer = outputCubeMap->GetOriginalBuffer();
	outputBuffer.resize(gpuBufferDesc.Width * gpuBufferDesc.Height * 6 * outputCubeMap->GetBytesPerPixel());

	UINT8* outputPixel = outputBuffer.data();
	for (UINT64 cubeSide = 0; cubeSide < 6; cubeSide++)
	{
		wxImage* inputImage = &inputImageArray[cubeSide];

		for (UINT64 i = 0; i < gpuBufferDesc.Height; i++)
		{
			for (UINT64 j = 0; j < gpuBufferDesc.Width; j++)
			{
				UINT pixelOffset = i * gpuBufferDesc.Width + j;

				const unsigned char* inputColor = &inputImage->GetData()[pixelOffset * bytesPerInputColorPixel];
				const unsigned char* inputAlpha = inputImage->HasAlpha() ? &inputImage->GetAlpha()[pixelOffset * bytesPerInputAlphaPixel] : nullptr;

				outputPixel[0] = inputColor[0];
				outputPixel[1] = inputColor[1];
				outputPixel[2] = inputColor[2];
				outputPixel[3] = inputAlpha ? inputAlpha[0] : 0;
				outputPixel += 4;
			}
		}
	}

	std::filesystem::path outputCubeMapPath = outputFolderPath / cubeMapName;
	outputCubeMapPath.replace_extension(".cube_map");

	if (!wxGetApp().GetGraphicsEngine()->DumpEnginePartToFile(outputCubeMapPath, outputCubeMap, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to dump cube map: %s", outputCubeMapPath.string().c_str());
		return false;
	}

	return true;
}