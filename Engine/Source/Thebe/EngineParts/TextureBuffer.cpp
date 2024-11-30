#include "Thebe/EngineParts/TextureBuffer.h"

using namespace Thebe;

TextureBuffer::TextureBuffer()
{
}

/*virtual*/ TextureBuffer::~TextureBuffer()
{
}

/*virtual*/ bool TextureBuffer::Setup()
{
	return true;
}

/*virtual*/ void TextureBuffer::Shutdown()
{
}

/*virtual*/ bool TextureBuffer::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& relativePath)
{
	return true;
}

/*virtual*/ bool TextureBuffer::DumpConfigurationToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue, const std::filesystem::path& relativePath) const
{
	return true;
}