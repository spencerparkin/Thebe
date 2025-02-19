#include "Thebe/AudioSystem.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/MidiSong.h"
#include "Thebe/EngineParts/AudioClip.h"

using namespace Thebe;

AudioSystem::AudioSystem()
{
}

/*virtual*/ AudioSystem::~AudioSystem()
{
}

bool AudioSystem::Setup()
{
	// TODO: Kick off MIDI thread here.

	// TODO: Initialize XAudio2 library.

	return true;
}

void AudioSystem::Shutdown()
{
	// TODO: Join MIDI thread here.

	this->midiSongMap.clear();
	this->audioClipMap.clear();
}

void AudioSystem::Update(double deltaTimeSeconds)
{
}

bool AudioSystem::LoadAudioFromFolder(const std::filesystem::path& folder, GraphicsEngine* graphicsEngine)
{
	std::filesystem::path folderResolved(folder);
	if (!graphicsEngine->ResolvePath(folderResolved, GraphicsEngine::RELATIVE_TO_ASSET_FOLDER))
		return false;

	for (const auto& entry : std::filesystem::directory_iterator(folderResolved))
	{
		if (!entry.is_regular_file())
			continue;

		std::string ext = entry.path().extension().string();
		
		if (ext == ".midi_song")
		{
			Reference<MidiSong> midiSong;
			if (graphicsEngine->LoadEnginePartFromFile(entry.path(), midiSong))
				this->midiSongMap.insert(std::pair(midiSong->GetName(), midiSong));
		}
		else if (ext == ".audio_clip")
		{
			Reference<AudioClip> audioClip;
			if (graphicsEngine->LoadEnginePartFromFile(entry.path(), audioClip))
				this->audioClipMap.insert(std::pair(audioClip->GetName(), audioClip));
		}
	}

	return true;
}

bool AudioSystem::EnqueueMidiSong(const std::string& midiSongName)
{
	return false;
}

bool AudioSystem::PlayWaveSoundNow(const std::string& waveSoundName)
{
	return false;
}

void AudioSystem::SetMidiSongVolume(double volume)
{
}

void AudioSystem::SetWaveSoundVolume(double volume)
{
}