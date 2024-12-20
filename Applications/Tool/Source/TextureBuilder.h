#pragma once

#include "App.h"
#include "Builder.h"
#include "Thebe/Log.h"
#include "Thebe/EngineParts/TextureBuffer.h"

class TextureBuilder : public Builder
{
public:
	TextureBuilder();
	virtual ~TextureBuilder();

	struct TextureBuildInfo
	{
		DXGI_FORMAT format;
		UINT maxMips;
	};

	void AddTexture(const std::filesystem::path& inputTexturePath, const TextureBuildInfo& textureBuildInfo);
	void ClearTextures();

	bool GenerateTextures(const std::filesystem::path& outputAssetsFolder);

	std::filesystem::path GenerateTextureBufferPath(const std::filesystem::path& inputTexturePath);

private:
	Thebe::Reference<Thebe::TextureBuffer> GenerateTextureBuffer(const std::filesystem::path& inputTexturePath, const TextureBuildInfo& buildInfo);

	std::map<std::filesystem::path, TextureBuildInfo> texturesToBuildMap;
	bool compressTextures;
};