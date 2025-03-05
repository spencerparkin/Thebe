#pragma once

#include "Thebe/Utilities/Clock.h"
#include "Thebe/Utilities/ScratchHeap.h"
#include "Thebe/Containers/LinkedList.h"
#include "Thebe/Reference.h"
#include <memory>
#include <map>
#if defined THEBE_USE_IMGUI
#	include <ImGui/imgui.h>
#	include <ImPlot/implot.h>
#	include "Thebe/ImGuiManager.h"
#endif //THEBE_USE_IMGUI

#define THEBE_NUM_GRAPH_PLOT_FRAMES		100

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

#if defined THEBE_USE_IMGUI
		void EnableImGuiProfilerWindow(bool enable, ImGuiManager* manager);
		bool ShowingImGuiProfilerWindow();
#endif //THEBE_USE_IMGUI

		class PersistentRecord : public ReferenceCounted
		{
		public:
			PersistentRecord();
			virtual ~PersistentRecord();

			void UpdateTree(const ProfileBlockRecord* blockRecord, uint64_t masterFrameKey);
			std::string GenerateText(int indentLevel = 0) const;

#if defined THEBE_USE_IMGUI
			void GenerateImGuiProfileTree() const;
			void GenerateImGuiPlotGraphs() const;
#endif //THEBE_USE_IMGUI

		public:
			const char* name;
			double timeTakenMilliseconds;
			std::map<std::string, Reference<PersistentRecord>> childMap;
			uint64_t frameKey;

#if defined THEBE_USE_IMGUI
			mutable std::list<double> plotDataHistory;
			mutable std::vector<const double*> plotDataArray;
			ImVec4 graphColor;
#endif //THEBE_USE_IMGUI
		};

		const PersistentRecord* GetProfileTree();

	private:

#if defined THEBE_USE_IMGUI
		void ShowImGuiProfilerWindow();
#endif //THEBE_USE_IMGUI

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
#if defined THEBE_USE_IMGUI
		int profilerWindowCookie;
#endif //THEBE_USE_IMGUI
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