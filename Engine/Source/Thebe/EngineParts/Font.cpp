#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/Log.h"

using namespace Thebe;

Font::Font()
{
	this->lineHeight = 0.0;
}

/*virtual*/ Font::~Font()
{
}

/*virtual*/ bool Font::Setup()
{
	if (!Material::Setup())
		return false;

	return true;
}

/*virtual*/ void Font::Shutdown()
{
	this->characterInfoArray.clear();

	Material::Shutdown();
}

/*virtual*/ bool Font::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!Material::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto characterArrayValue = dynamic_cast<const JsonArray*>(rootValue->GetValue("character_array"));
	if (!characterArrayValue)
	{
		THEBE_LOG("Expected character array value to be present in the JSON.");
		return false;
	}

	double minY = std::numeric_limits<double>::max();
	double maxY = std::numeric_limits<double>::min();

	this->characterInfoArray.clear();
	for (UINT i = 0; i < (UINT)characterArrayValue->GetSize(); i++)
	{
		CharacterInfo charInfo;

		auto characterInfoValue = dynamic_cast<const JsonObject*>(characterArrayValue->GetValue(i));
		if (!characterInfoValue)
		{
			THEBE_LOG("Expected each character array element to be a JSON object.");
			return false;
		}

		auto advanceValue = dynamic_cast<const JsonFloat*>(characterInfoValue->GetValue("advance"));
		if (!advanceValue)
		{
			THEBE_LOG("No advance value found.");
			return false;
		}

		charInfo.advance = advanceValue->GetValue();

		if (!JsonHelper::VectorFromJsonValue(dynamic_cast<const JsonObject*>(characterInfoValue->GetValue("min")), charInfo.minUV))
		{
			THEBE_LOG("Failed to load min UVs.");
			return false;
		}

		if (!JsonHelper::VectorFromJsonValue(dynamic_cast<const JsonObject*>(characterInfoValue->GetValue("max")), charInfo.maxUV))
		{
			THEBE_LOG("Failed to load max UVs.");
			return false;
		}

		if (!JsonHelper::VectorFromJsonValue(dynamic_cast<const JsonObject*>(characterInfoValue->GetValue("pen_offset")), charInfo.penOffset))
		{
			THEBE_LOG("Failed to load pen offset.");
			return false;
		}

		this->characterInfoArray.push_back(charInfo);

		maxY = THEBE_MAX(maxY, charInfo.penOffset.y + (charInfo.maxUV.y - charInfo.minUV.y));
		minY = THEBE_MIN(minY, charInfo.penOffset.y);
	}

	this->lineHeight = maxY - minY;

	return true;
}

/*virtual*/ bool Font::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& assetPath) const
{
	using namespace ParseParty;

	if (!Material::DumpConfigurationToJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<JsonObject*>(jsonValue.get());
	if (!rootValue)
		return false;

	auto characterArrayValue = new JsonArray();
	rootValue->SetValue("character_array", characterArrayValue);
	for (UINT i = 0; i < (UINT)this->characterInfoArray.size(); i++)
	{
		const CharacterInfo& charInfo = this->characterInfoArray[i];
		auto characterInfoValue = new JsonObject();
		characterArrayValue->PushValue(characterInfoValue);
		characterInfoValue->SetValue("advance", new JsonFloat(charInfo.advance));
		characterInfoValue->SetValue("min", JsonHelper::VectorToJsonValue(charInfo.minUV));
		characterInfoValue->SetValue("max", JsonHelper::VectorToJsonValue(charInfo.maxUV));
		characterInfoValue->SetValue("pen_offset", JsonHelper::VectorToJsonValue(charInfo.penOffset));
	}

	return true;
}

std::vector<Font::CharacterInfo>& Font::GetCharacterInfoArray()
{
	return this->characterInfoArray;
}

const std::vector<Font::CharacterInfo>& Font::GetCharacterInfoArray() const
{
	return this->characterInfoArray;
}

double Font::GetLineHeight() const
{
	return this->lineHeight;
}