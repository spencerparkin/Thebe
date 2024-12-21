#include "Thebe/EngineParts/Font.h"
#include "Thebe/EngineParts/TextureBuffer.h"
#include "Thebe/EngineParts/VertexBuffer.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/Log.h"

using namespace Thebe;

Font::Font()
{
}

/*virtual*/ Font::~Font()
{
}

/*virtual*/ bool Font::Setup()
{
	if (!Material::Setup())
		return false;

	this->vertexBuffer.Set(new VertexBuffer());

	std::vector<D3D12_INPUT_ELEMENT_DESC>& elementDescArray = this->vertexBuffer->GetElementDescArray();
	elementDescArray.resize(2);
	elementDescArray[0].AlignedByteOffset = 0;
	elementDescArray[0].Format = DXGI_FORMAT_R32G32_FLOAT;
	elementDescArray[0].InputSlot = 0;
	elementDescArray[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	elementDescArray[0].InstanceDataStepRate = 0;
	elementDescArray[0].SemanticIndex = 0;
	elementDescArray[0].SemanticName = "POSITION";
	elementDescArray[1].AlignedByteOffset = 0;
	elementDescArray[1].Format = DXGI_FORMAT_UNKNOWN;
	elementDescArray[1].InputSlot = 1;
	elementDescArray[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
	elementDescArray[1].InstanceDataStepRate = 0;
	elementDescArray[1].SemanticIndex = 0;
	elementDescArray[1].SemanticName = "CHAR_INFO";

	this->vertexBuffer->SetStride(THEBE_ALIGNED(2 * sizeof(float), 16));
	this->vertexBuffer->SetBufferType(Buffer::STATIC);

	std::vector<UINT8>& originalBuffer = this->vertexBuffer->GetOriginalBuffer();
	originalBuffer.resize(6 * 2 * sizeof(float));
	auto vertex = reinterpret_cast<Vector2*>(originalBuffer.data());
	vertex[0].SetComponents(0.0, 0.0);
	vertex[1].SetComponents(1.0, 0.0);
	vertex[2].SetComponents(1.0, 1.0);
	vertex[3].SetComponents(0.0, 0.0);
	vertex[4].SetComponents(1.0, 1.0);
	vertex[5].SetComponents(0.0, 1.0);

	if (!this->vertexBuffer->Setup())
	{
		THEBE_LOG("Failed to setup vertex buffer for tex instance.");
		return false;
	}

	return true;
}

/*virtual*/ void Font::Shutdown()
{
	if (this->vertexBuffer.Get())
	{
		this->vertexBuffer->Shutdown();
		this->vertexBuffer = nullptr;
	}

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
	}

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

VertexBuffer* Font::GetVertexBuffer()
{
	return this->vertexBuffer;
}