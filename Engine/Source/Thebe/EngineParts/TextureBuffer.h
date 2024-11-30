#pragma once

#include "Thebe/EngineParts/Buffer.h"

namespace Thebe
{
	// TODO: Can we make a class that derives from the RenderTarget class that
	//       is capable of letting us render into a texture that we can then
	//       later use to texture map a mesh in the main rendering pass?

	/**
	 * 
	 */
	class THEBE_API TextureBuffer : public Buffer
	{
	public:
		TextureBuffer();
		virtual ~TextureBuffer();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const override;
	};
}