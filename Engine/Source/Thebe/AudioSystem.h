#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"
#include "Thebe/Utilities/Thread.h"
#include "Thebe/EventSystem.h"
#include "AudioDataLib/MIDI/MidiPlayer.h"
#include "AudioDataLib/Timer.h"
#include <RtMidi.h>
#include <filesystem>
#include <unordered_map>
#include <semaphore>

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
		AudioSystem(EventSystem* eventSystem);
		virtual ~AudioSystem();

		bool Setup();
		void Shutdown();
		void Update(double deltaTimeSeconds);

		bool LoadAudioFromFolder(const std::filesystem::path& folder, GraphicsEngine* graphicsEngine);

		bool EnqueueMidiSong(const std::string& midiSongName);
		bool ChangeMidiVolume(double volume);

		bool PlayWaveSoundNow(const std::string& waveSoundName);

		void SetMidiSongVolume(double volume);
		void SetWaveSoundVolume(double volume);

	private:

		/**
		 *
		 */
		class MidiThread : public Thread
		{
			friend class EnqueueSongTask;
			friend class ChangeVolumeTask;

		public:
			MidiThread(EventSystem* eventSystem);
			virtual ~MidiThread();

			virtual bool Split() override;
			virtual bool Join() override;
			virtual void Run() override;

			class Task;

			void EnqueueTask(Task* task);

			class Task : public ReferenceCounted
			{
			public:
				virtual void Perform(MidiThread* thread) = 0;
			};

			class EnqueueSongTask : public Task
			{
			public:
				virtual void Perform(MidiThread* thread) override;
				Reference<MidiSong> midiSong;
			};

			class ChangeVolumeTask : public Task
			{
			public:
				ChangeVolumeTask(double volume);
				virtual void Perform(MidiThread* thread) override;
				double volume;
			};

		private:
			std::list<Reference<Task>> taskQueue;
			std::mutex taskQueueMutex;
			std::counting_semaphore<std::numeric_limits<uint32_t>::max()> taskQueueSemaphore;
			std::unique_ptr<RtMidiOut> midiOut;
			std::list<Reference<MidiSong>> midiSongQueue;
			Reference<MidiSong> currentlyPlayingSong;
			AudioDataLib::SystemClockTimer timer;
			AudioDataLib::MidiPlayer player;
			bool exitSignaled;
			EventSystem* eventSystem;
		};

		std::unordered_map<std::string, Reference<MidiSong>> midiSongMap;
		std::unordered_map<std::string, Reference<AudioClip>> audioClipMap;
		std::unique_ptr<MidiThread> midiThread;
		EventSystem* eventSystem;
	};

	/**
	 * 
	 */
	class THEBE_API AudioEvent : public Event
	{
	public:
		AudioEvent();
		virtual ~AudioEvent();

		enum Type
		{
			UNKNOWN,
			SONG_STARTED,
			SONG_ENDED,
			SONG_QUEUE_EMPTY,
			WAVE_STARTED,
			WAVE_ENDED
		};

		Type type;
		std::string extraInfo;
	};
}