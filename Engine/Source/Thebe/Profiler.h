#pragma once

#include "Thebe/Utilities/Clock.h"
#include "Thebe/Utilities/ScratchHeap.h"
#include "Thebe/Containers/LinkedList.h"
#include "Thebe/Reference.h"
#include <memory>
#include <map>
#include <ImGui/imgui.h>
#include <ImPlot/implot.h>
#include "Thebe/ImGuiManager.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API Profiler
	{
		friend class ScopedProfileBlock;
		
	public:
		class ProfileBlockRecord;

		Profiler();
		virtual ~Profiler();

		static Profiler* Get();

		void BeginFrame();
		void EndFrame();

		void EnableImGuiProfilerWindow(bool enable, ImGuiManager* manager);
		bool ShowingImGuiProfilerWindow();

		class PersistentRecord : public ReferenceCounted
		{
		public:
			PersistentRecord();
			virtual ~PersistentRecord();

			void UpdateTree(const ProfileBlockRecord* blockRecord, uint64_t masterFrameKey);
			std::string GenerateText(int indentLevel = 0) const;

			void GenerateImGuiPlotGraphs(int numGraphPlotFrames) const;

		public:
			const char* name;
			double timeTakenMilliseconds;
			std::map<std::string, Reference<PersistentRecord>> childMap;
			uint64_t frameKey;
			mutable std::list<double> plotDataHistory;
			mutable std::vector<const double*> plotDataArray;
			ImVec4 graphColor;
		};

		const PersistentRecord* GetProfileTree();

	private:

		void ShowImGuiProfilerWindow();

		class ProfileBlockRecord : public LinkedListNode
		{
		public:
			ProfileBlockRecord();
			virtual ~ProfileBlockRecord();

		public:
			const char* name;
			double timeTakenMilliseconds;
			LinkedList nestedBlockList;
			ProfileBlockRecord* parentBlock;
		};

		void ProfileBlockBegin(const char* name);
		void ProfileBlockEnd(double timeTakenMilliseconds);

		ObjectScratchHeap<ProfileBlockRecord> blockRecordHeap;
		std::unique_ptr<uint8_t> blockRecordHeapMemory;
		ProfileBlockRecord* rootRecord;
		ProfileBlockRecord* currentRecord;
		Clock clock;
		Reference<PersistentRecord> persistentRootRecord;
		uint64_t frameKey;
		int profilerWindowCookie;
		int numGraphPlotFrames;
	};

	/**
	 * 
	 */
	class THEBE_API ScopedProfileBlock
	{
	public:
		ScopedProfileBlock(const char* name);
		virtual ~ScopedProfileBlock();

	private:
		Clock clock;
	};
}

#if defined THEBE_PROFILING
#	define THEBE_PROFILE_BLOCK(name)	ScopedProfileBlock block_##name(#name)
#	define THEBE_PROFILE_BEGIN_FRAME	Profiler::Get()->BeginFrame()
#	define THEBE_PROFILE_END_FRAME		Profiler::Get()->EndFrame()
#else
#	define THEBE_PROFILE_BLOCK(name)
#	define THEBE_PROFILE_BEGIN_FRAME
#	define THEBE_PROFILE_END_FRAME
#endif