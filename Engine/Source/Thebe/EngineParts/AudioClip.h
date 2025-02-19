#pragma once

#include "Thebe/EnginePart.h"
#include "AudioDataLib/FileDatas/AudioData.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API AudioClip : public EnginePart
	{
	public:
		AudioClip();
		virtual ~AudioClip();

		virtual bool Setup() override;
		virtual bool LoadConfigurationFromJson(const ParseParty::JsonValue* jsonValue, const std::filesystem::path& assetPath) override;

	protected:
		std::shared_ptr<AudioDataLib::AudioData> audioData;
		std::filesystem::path audioDataPath;
	};
}