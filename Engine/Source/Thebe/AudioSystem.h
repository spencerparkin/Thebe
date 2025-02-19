#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include <filesystem>
#include <unordered_map>

namespace Thebe
{
	class AudioClip;
	class MidiSong;
	class GraphicsEngine;

	/**
	 * This is our audio abstraction layer.
	 */
	class THEBE_API AudioSystem
	{
	public:
		AudioSystem();
		virtual ~AudioSystem();

		bool Setup();
		void Shutdown();
		void Update(double deltaTimeSeconds);

		bool LoadAudioFromFolder(const std::filesystem::path& folder, GraphicsEngine* graphicsEngine);

		bool EnqueueMidiSong(const std::string& midiSongName);
		bool PlayWaveSoundNow(const std::string& waveSoundName);

		void SetMidiSongVolume(double volume);
		void SetWaveSoundVolume(double volume);

	private:

		std::unordered_map<std::string, Reference<MidiSong>> midiSongMap;
		std::unordered_map<std::string, Reference<AudioClip>> audioClipMap;
	};
}