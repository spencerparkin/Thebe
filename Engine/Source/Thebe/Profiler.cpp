#include "Thebe/Profiler.h"
#include "Thebe/Math/Random.h"
#include <format>

using namespace Thebe;

//------------------------------------ Profiler ------------------------------------

Profiler::Profiler()
{
	this->persistentRootRecord = nullptr;
	this->rootRecord = nullptr;
	this->currentRecord = nullptr;
	this->blockRecordHeap.SetSize(sizeof(ProfileBlockRecord) * 2048);
	this->frameKey = 0;
#if defined THEBE_USE_IMGUI
	this->profilerWindowCookie = 0;
#endif //THEBE_USE_IMGUI
}

/*virtual*/ Profiler::~Profiler()
{
	delete this->persistentRootRecord;
}

/*static*/ Profiler* Profiler::Get()
{
	static Profiler profiler;
	return &profiler;
}

void Profiler::BeginFrame()
{
	this->frameKey++;
	this->rootRecord = this->blockRecordHeap.AllocateObject();
	this->rootRecord->name = "Frame";
	this->currentRecord = this->rootRecord;
	this->clock.Reset();
}

void Profiler::EndFrame()
{
	THEBE_ASSERT(this->currentRecord == this->rootRecord);
	this->rootRecord->timeTakenMilliseconds = this->clock.GetCurrentTimeMilliseconds();

	if (!this->persistentRootRecord)
		this->persistentRootRecord = new PersistentRecord();

	this->persistentRootRecord->UpdateTree(this->rootRecord, this->frameKey);

	this->blockRecordHeap.Reset();
}

void Profiler::ProfileBlockBegin(const char* name)
{
	if (!this->currentRecord)
		return;

	ProfileBlockRecord* newRecord = this->blockRecordHeap.AllocateObject();
	newRecord->name = name;
	newRecord->parentBlock = this->currentRecord;
	this->currentRecord->nestedBlockList.InsertNodeAfter(newRecord);
	this->currentRecord = newRecord;
}

void Profiler::ProfileBlockEnd(double timeTakenMilliseconds)
{
	if (!this->currentRecord)
		return;

	this->currentRecord->timeTakenMilliseconds = timeTakenMilliseconds;
	this->currentRecord = this->currentRecord->parentBlock;
}

const Profiler::PersistentRecord* Profiler::GetProfileTree()
{
	return this->persistentRootRecord;
}

#if defined THEBE_USE_IMGUI

void Profiler::EnableImGuiProfilerWindow(bool enable, ImGuiManager* manager)
{
	if (enable)
	{
		if (this->profilerWindowCookie == 0)
			manager->RegisterGuiCallback([this]() { this->ShowImGuiProfilerWindow(); }, this->profilerWindowCookie);
	}
	else
	{
		if (this->profilerWindowCookie != 0)
			manager->UnregisterGuiCallback(this->profilerWindowCookie);
	}
}

bool Profiler::ShowingImGuiProfilerWindow()
{
	return this->profilerWindowCookie != 0;
}

void Profiler::ShowImGuiProfilerWindow()
{
	ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Profiler Graph"))
	{
		if (this->persistentRootRecord)
		{
			ImPlot::SetNextAxesLimits(0.0, double(THEBE_NUM_GRAPH_PLOT_FRAMES - 1), 0.0, 30.0, ImPlotCond_Once);

			if (ImPlot::BeginPlot("Frame Times", "Frame", "Milliseconds"))
			{
				this->persistentRootRecord->GenerateImGuiPlotGraphs();

				ImPlot::EndPlot();
			}

			this->persistentRootRecord->GenerateImGuiProfileTree();
		}

		ImGui::End();
	}
}

#endif //THEBE_USE_IMGUI

//------------------------------------ Profiler::ProfileBlockRecord ------------------------------------

Profiler::ProfileBlockRecord::ProfileBlockRecord()
{
	this->parentBlock = nullptr;
	this->name = nullptr;
	this->timeTakenMilliseconds = 0.0;
}

/*virtual*/ Profiler::ProfileBlockRecord::~ProfileBlockRecord()
{
}

//------------------------------------ Profiler::PersistentRecord ------------------------------------

Profiler::PersistentRecord::PersistentRecord()
{
	static Random random;

	this->name = nullptr;
	this->timeTakenMilliseconds = 0.0;
	this->frameKey = 0;
#if defined THEBE_USE_IMGUI
	this->graphColor.x = random.InRange(0.0, 1.0);
	this->graphColor.y = random.InRange(0.0, 1.0);
	this->graphColor.z = random.InRange(0.0, 1.0);
	this->graphColor.w = 1.0;
#endif //THEBE_USE_IMGUI
}

/*virtual*/ Profiler::PersistentRecord::~PersistentRecord()
{
}

void Profiler::PersistentRecord::UpdateTree(const ProfileBlockRecord* blockRecord, uint64_t masterFrameKey)
{
	this->name = blockRecord->name;

	if (this->frameKey != masterFrameKey)
	{
		this->frameKey = masterFrameKey;
		this->timeTakenMilliseconds = 0.0;
	}

	this->timeTakenMilliseconds += blockRecord->timeTakenMilliseconds;

	for (const LinkedListNode* node = blockRecord->nestedBlockList.GetHeadNode(); node; node = node->GetNextNode())
	{
		auto nestedRecord = dynamic_cast<const ProfileBlockRecord*>(node);

		PersistentRecord* childRecord = nullptr;
		auto pair = this->childMap.find(nestedRecord->name);
		if (pair != this->childMap.end())
			childRecord = pair->second.Get();
		else
		{
			childRecord = new PersistentRecord();
			this->childMap.insert(std::pair(nestedRecord->name, childRecord));
		}

		childRecord->UpdateTree(nestedRecord, masterFrameKey);
	}
}

std::string Profiler::PersistentRecord::GenerateText(int indentLevel /*= 0*/) const
{
	std::string text = std::format("{}: {:2.2f}\n", this->name, this->timeTakenMilliseconds);

	for (int i = 0; i < indentLevel; i++)
		text = "   " + text;

	for (const auto& pair : this->childMap)
	{
		std::string subText = pair.second->GenerateText(indentLevel + 1);
		text += subText;
	}

	return text;
}

#if defined THEBE_USE_IMGUI
void Profiler::PersistentRecord::GenerateImGuiProfileTree() const
{
	if (ImGui::TreeNode(this->name))
	{
		ImGui::LabelText("Time Taken", "%1.2f ms", this->timeTakenMilliseconds);

		for (const auto& pair : this->childMap)
			pair.second->GenerateImGuiProfileTree();

		ImGui::TreePop();
	}
}

void Profiler::PersistentRecord::GenerateImGuiPlotGraphs() const
{
	this->plotDataHistory.push_back(this->timeTakenMilliseconds);
	while (this->plotDataHistory.size() > THEBE_NUM_GRAPH_PLOT_FRAMES)
		this->plotDataHistory.pop_front();

	this->plotDataArray.clear();
	for (const double& timeTakenEntry : this->plotDataHistory)
		this->plotDataArray.push_back(&timeTakenEntry);

	ImPlot::SetNextLineStyle(this->graphColor);

	ImPlot::PlotLineG(this->name, [](int index, void* data)
		{
			const auto* persistentRecord = reinterpret_cast<const PersistentRecord*>(data);
			ImPlotPoint point;
			point.x = (double)index;
			point.y = *persistentRecord->plotDataArray[index];
			return point;
		}, const_cast<PersistentRecord*>(this), (int)this->plotDataArray.size());

	for (const auto& pair : this->childMap)
		pair.second->GenerateImGuiPlotGraphs();
}
#endif //THEBE_USE_IMGUI

//------------------------------------ ScopedProfileBlock ------------------------------------

ScopedProfileBlock::ScopedProfileBlock(const char* name)
{
	Profiler::Get()->ProfileBlockBegin(name);
	this->clock.Reset();
}

/*virtual*/ ScopedProfileBlock::~ScopedProfileBlock()
{
	double timeTakenMilliseconds = this->clock.GetCurrentTimeMilliseconds();
	Profiler::Get()->ProfileBlockEnd(timeTakenMilliseconds);
}