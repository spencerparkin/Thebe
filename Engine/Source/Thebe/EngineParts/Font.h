#pragma once

#include "Thebe/EngineParts/Material.h"
#include "Thebe/Math/Vector2.h"

namespace Thebe
{
	class VertexBuffer;

	/**
	 * 
	 */
	class THEBE_API Font : public Material
	{
	public:
		Font();
		virtual ~Font();

		virtual bool Setup() override;
		virtual void Shutdown() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;
		virtual bool DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const override;

		struct CharacterInfo
		{
			double advance;
			Vector2 minUV;
			Vector2 maxUV;
			Vector2 penOffset;
		};

		std::vector<CharacterInfo>& GetCharacterInfoArray();
		const std::vector<CharacterInfo>& GetCharacterInfoArray() const;

		VertexBuffer* GetVertexBuffer();

	private:
		std::vector<CharacterInfo> characterInfoArray;
		Reference<VertexBuffer> vertexBuffer;
	};
}