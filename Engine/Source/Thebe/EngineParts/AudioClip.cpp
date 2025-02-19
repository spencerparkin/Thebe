#include "Thebe/EngineParts/AudioClip.h"
#include "AudioDataLib/FileFormats/FileFormat.h"
#include "AudioDataLib/ByteStream.h"

using namespace Thebe;

AudioClip::AudioClip()
{
}

/*virtual*/ AudioClip::~AudioClip()
{
}

/*virtual*/ bool AudioClip::Setup()
{
	using namespace AudioDataLib;

	std::shared_ptr<FileFormat> fileFormat = FileFormat::CreateForFile(this->audioDataPath.string());
	if (!fileFormat.get())
		return false;

	FileInputStream inputStream(this->audioDataPath.string().c_str());
	if (!inputStream.IsOpen())
		return false;

	std::unique_ptr<FileData> fileData;
	if (!fileFormat->ReadFromStream(inputStream, fileData))
		return false;

	this->audioData.reset(dynamic_cast<AudioData*>(fileData.get()));
	if (!this->audioData.get())
		return false;

	fileData.release();
	return true;
}

/*virtual*/ bool AudioClip::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!EnginePart::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto clipPathValue = dynamic_cast<const JsonString*>(rootValue->GetValue("clip_path"));
	if (!clipPathValue)
		return false;

	this->audioDataPath = clipPathValue->GetValue();
	this->audioDataPath = assetPath.parent_path() / this->audioDataPath;
	return true;
}