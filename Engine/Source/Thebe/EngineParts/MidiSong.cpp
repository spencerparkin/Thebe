#include "Thebe/EngineParts/MidiSong.h"
#include "AudioDataLib/FileFormats/FileFormat.h"
#include "AudioDataLib/ByteStream.h"
#include "AudioDataLib/ErrorSystem.h"
#include "Thebe/Log.h"

using namespace Thebe;

MidiSong::MidiSong()
{
}

/*virtual*/ MidiSong::~MidiSong()
{
}

/*virtual*/ bool MidiSong::Setup()
{
	using namespace AudioDataLib;

	std::shared_ptr<FileFormat> fileFormat = FileFormat::CreateForFile(this->midiDataPath.string());
	if (!fileFormat.get())
		return false;

	FileInputStream inputStream(this->midiDataPath.string().c_str());
	if (!inputStream.IsOpen())
		return false;

	std::unique_ptr<FileData> fileData;
	if (!fileFormat->ReadFromStream(inputStream, fileData))
	{
		THEBE_LOG(ErrorSystem::Get()->GetErrorMessage().c_str());
		return false;
	}

	this->midiData.reset(dynamic_cast<MidiData*>(fileData.get()));
	if (!this->midiData.get())
		return false;

	fileData.release();
	return true;
}

/*virtual*/ bool MidiSong::LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath)
{
	using namespace ParseParty;

	if (!EnginePart::LoadConfigurationFromJson(jsonValue, assetPath))
		return false;

	auto rootValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!rootValue)
		return false;

	auto midiSongPathValue = dynamic_cast<const JsonString*>(rootValue->GetValue("midi_song_path"));
	if (!midiSongPathValue)
		return false;

	this->midiDataPath = midiSongPathValue->GetValue();
	this->midiDataPath = assetPath.parent_path() / this->midiDataPath;
	return true;
}