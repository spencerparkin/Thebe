#include "Thebe/AudioSystem.h"
#include "Thebe/GraphicsEngine.h"
#include "Thebe/EngineParts/MidiSong.h"
#include "Thebe/EngineParts/AudioClip.h"
#include "Thebe/Log.h"
#include "AudioDataLib/MIDI/MidiMsgDestination.h"
#include "AudioDataLib/ErrorSystem.h"

using namespace Thebe;

//------------------------------------ AudioSystem ------------------------------------

AudioSystem::AudioSystem(EventSystem* eventSystem)
{
	this->eventSystem = eventSystem;
}

/*virtual*/ AudioSystem::~AudioSystem()
{
}

bool AudioSystem::Setup()
{
	// TODO: Initialize XAudio2 library.

	this->midiThread.reset(new MidiThread(this->eventSystem));
	if (!this->midiThread->Split())
		return false;

	return true;
}

void AudioSystem::Shutdown()
{
	if (this->midiThread)
	{
		this->midiThread->Join();
		this->midiThread.reset(nullptr);
	}

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
	if (!this->midiThread.get())
		return false;

	auto pair = this->midiSongMap.find(midiSongName);
	if (pair == this->midiSongMap.end())
		return false;

	auto task = new MidiThread::EnqueueSongTask();
	task->midiSong = pair->second;
	this->midiThread->EnqueueTask(task);
	return true;
}

bool AudioSystem::ChangeMidiVolume(double volume)
{
	if (!this->midiThread.get())
		return false;

	auto task = new MidiThread::ChangeVolumeTask(volume);
	this->midiThread->EnqueueTask(task);
	return true;
}

bool AudioSystem::PlayWaveSoundNow(const std::string& waveSoundName)
{
	return false;
}

void AudioSystem::SetWaveSoundVolume(double volume)
{
}

//------------------------------------ AudioSystem::MidiThread ------------------------------------

AudioSystem::MidiThread::MidiThread(EventSystem* eventSystem) : Thread("AudioSystem::MidiThread"), taskQueueSemaphore(0), player(&this->timer)
{
	this->exitSignaled = false;
	this->eventSystem = eventSystem;
}

/*virtual*/ AudioSystem::MidiThread::~MidiThread()
{
}

/*virtual*/ bool AudioSystem::MidiThread::Split()
{
	try
	{
		this->midiOut.reset(new RtMidiOut());

		unsigned int numPorts = this->midiOut->getPortCount();
		if (numPorts == 0)
		{
			THEBE_LOG("No MIDI ports found!");
			return false;
		}

		for (unsigned int i = 0; i < numPorts; i++)
			THEBE_LOG("MIDI port %d is \"%s\".", i, this->midiOut->getPortName(i).c_str());

		// TODO: Which port should we choose?  For now, just choose the first one.
		this->midiOut->openPort(0);
	}
	catch (RtMidiError& midiError)
	{
		THEBE_LOG("RtMidi error: %s", midiError.getMessage().c_str());
	}

	if (!this->midiOut->isPortOpen())
		return false;

	return Thread::Split();
}

/*virtual*/ bool AudioSystem::MidiThread::Join()
{
	this->exitSignaled = true;
	this->taskQueueSemaphore.release();
	return Thread::Join();
}

void AudioSystem::MidiThread::EnqueueTask(Task* task)
{
	std::scoped_lock lock(this->taskQueueMutex);
	this->taskQueue.push_back(task);
	this->taskQueueSemaphore.release();
}

/*virtual*/ void AudioSystem::MidiThread::Run()
{
	using namespace AudioDataLib;

	class RtMidiMsgDestination : public MidiMsgDestination
	{
	public:
		RtMidiMsgDestination(RtMidiOut* midiOut)
		{
			this->midiOut = midiOut;
		}

		virtual bool ReceiveMessage(double deltaTimeSeconds, const uint8_t* message, uint64_t messageSize) override
		{
			try
			{
				this->midiOut->sendMessage(message, messageSize);
			}
			catch (RtMidiError& midiError)
			{
				THEBE_LOG("RtMidi error: %s", midiError.getMessage().c_str());
				return false;
			}

			return true;
		}

		RtMidiOut* midiOut;
	};

	std::shared_ptr<RtMidiMsgDestination> midiDestination(new RtMidiMsgDestination(this->midiOut.get()));
	this->player.Clear();
	this->player.AddDestination(midiDestination);

	while (!this->exitSignaled)
	{
		if (!this->currentlyPlayingSong)
		{
			if (this->midiSongQueue.size() == 0)
				this->taskQueueSemaphore.acquire();		// Don't busy wait if there is nothing for us to do.
			else
			{
				this->currentlyPlayingSong = this->midiSongQueue.front();
				this->midiSongQueue.pop_front();

				this->player.SetMidiData(this->currentlyPlayingSong->GetMidiData());
				this->player.ConfigureToPlayAllTracks();

				if (!this->player.Setup())
				{
					THEBE_LOG("Failed to setup MIDI player!");
					THEBE_LOG(ErrorSystem::Get()->GetErrorMessage().c_str());
					break;
				}

				auto audioEvent = new AudioEvent();
				audioEvent->type = AudioEvent::SONG_STARTED;
				audioEvent->extraInfo = this->currentlyPlayingSong->GetName();
				this->eventSystem->SendEvent(audioEvent);
			}
		}

		if (this->taskQueue.size() > 0)
		{
			Reference<Task> task;
		
			std::scoped_lock lock(this->taskQueueMutex);
			if (this->taskQueue.size() > 0)
			{
				task = this->taskQueue.front();
				this->taskQueue.pop_front();
			}

			task->Perform(this);
		}

		if (this->currentlyPlayingSong)
		{
			if (this->player.NoMoreToPlay())
			{
				this->player.Shutdown();

				auto audioEvent = new AudioEvent();
				audioEvent->type = AudioEvent::SONG_ENDED;
				audioEvent->extraInfo = this->currentlyPlayingSong->GetName();
				this->eventSystem->SendEvent(audioEvent);

				if (this->midiSongQueue.size() == 0)
				{
					audioEvent = new AudioEvent();
					audioEvent->type = AudioEvent::SONG_QUEUE_EMPTY;
					this->eventSystem->SendEvent(audioEvent);
				}

				this->currentlyPlayingSong = nullptr;
			}
			else if (!this->player.Process())
			{
				THEBE_LOG("MIDI player processing failed!");
				THEBE_LOG(ErrorSystem::Get()->GetErrorMessage().c_str());
				break;
			}
		}
	}
}

//------------------------------------ AudioSystem::MidiThread::EnqueueSongTask ------------------------------------

/*virtual*/ void AudioSystem::MidiThread::EnqueueSongTask::Perform(MidiThread* thread)
{
	thread->midiSongQueue.push_back(this->midiSong);
}

//------------------------------------ AudioSystem::MidiThread::ChangeVolumeTask ------------------------------------

AudioSystem::MidiThread::ChangeVolumeTask::ChangeVolumeTask(double volume)
{
	this->volume = volume;
}

/*virtual*/ void AudioSystem::MidiThread::ChangeVolumeTask::Perform(MidiThread* thread)
{
	using namespace AudioDataLib;

	MidiData::SystemExclusiveEvent event;
	event.SetAsMasterVolumeEvent(this->volume);

	uint8_t messageBuffer[ADL_MIDI_MESSAGE_BUFFER_SIZE];
	WriteOnlyBufferStream outputStream(messageBuffer, sizeof(messageBuffer));
	event.Encode(outputStream);

	thread->midiOut->sendMessage(outputStream.GetBuffer(), outputStream.GetSize());
}

//------------------------------------ AudioEvent ------------------------------------

AudioEvent::AudioEvent()
{
	this->type = Type::UNKNOWN;
	this->category = "Audio";
}

/*virtual*/ AudioEvent::~AudioEvent()
{
}