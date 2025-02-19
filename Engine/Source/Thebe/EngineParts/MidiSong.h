#pragma once

#include "Thebe/EnginePart.h"
#include "AudioDataLib/FileDatas/MidiData.h"

namespace Thebe
{
	/**
	 *
	 */
	class THEBE_API MidiSong : public EnginePart
	{
	public:
		MidiSong();
		virtual ~MidiSong();

		virtual bool Setup() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;

	protected:
		std::shared_ptr<AudioDataLib::MidiData> midiData;
		std::filesystem::path midiDataPath;
	};
}