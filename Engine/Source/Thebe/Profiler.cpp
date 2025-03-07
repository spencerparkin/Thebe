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
	this->profilerWindowCookie = 0;
	this->numGraphPlotFrames = 100;
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

void Profiler::RegisterWithImGuiManager()
{
	ImGuiManager::Get()->RegisterGuiCallback([this]() { this->ShowImGuiProfilerWindow(); }, this->profilerWindowCookie);
}

void Profiler::EnableImGuiProfilerWindow(bool enable)
{
	ImGuiManager::Get()->EnableGuiCallback(this->profilerWindowCookie, enable);
}

bool Profiler::ShowingImGuiProfilerWindow()
{
	return ImGuiManager::Get()->IsGuiCallbackEnabled(this->profilerWindowCookie);
}

void Profiler::ShowImGuiProfilerWindow()
{
	ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Profiler Graph"))
	{
		if (this->persistentRootRecord)
		{
			ImPlot::SetNextAxesLimits(0.0, double(this->numGraphPlotFrames - 1), 0.0, 30.0, ImPlotCond_Always);

			if (ImPlot::BeginPlot("Frame Times", "Frame", "Milliseconds"))
			{
				this->persistentRootRecord->GenerateImGuiPlotGraphs(this->numGraphPlotFrames);

				ImPlot::EndPlot();
			}

			ImGui::SliderInt("Num Frames", &this->numGraphPlotFrames, 10, 500);
		}
	}

	ImGui::End();
}

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
	this->graphColor.x = random.InRange(0.0, 1.0);
	this->graphColor.y = random.InRange(0.0, 1.0);
	this->graphColor.z = random.InRange(0.0, 1.0);
	this->graphColor.w = 1.0;
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

void Profiler::PersistentRecord::GenerateImGuiPlotGraphs(int numGraphPlotFrames) const
{
	this->plotDataHistory.push_back(this->timeTakenMilliseconds);
	while (this->plotDataHistory.size() > numGraphPlotFrames)
		this->plotDataHistory.pop_front();

	this->plotDataArray.clear();
	static double zero = 0.0;
	for (int i = 0; i < numGraphPlotFrames - int(this->plotDataHistory.size()); i++)
		this->plotDataArray.push_back(&zero);
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
		pair.second->GenerateImGuiPlotGraphs(numGraphPlotFrames);
}

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