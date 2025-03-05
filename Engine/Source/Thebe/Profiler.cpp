#include "Profiler.h"
#include <format>
#if defined THEBE_USE_IMGUI
#	include <ImGui/imgui.h>
#endif //THEBE_USE_IMGUI

using namespace Thebe;

//------------------------------------ Profiler ------------------------------------

Profiler::Profiler()
{
	this->persistentRootRecord = nullptr;
	this->rootRecord = nullptr;
	this->currentRecord = nullptr;
	this->blockRecordHeap.SetSize(sizeof(ProfileBlockRecord) * 2048);
	this->frameKey = 0;
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
void Profiler::ShowImGuiPlotTreeWindow()
{
	ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Profiler Plots"))
	{
		if (this->persistentRootRecord)
			this->persistentRootRecord->GenerateImGuiPlotTree();

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
	this->name = nullptr;
	this->timeTakenMilliseconds = 0.0;
	this->frameKey = 0;
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
void Profiler::PersistentRecord::GenerateImGuiPlotTree() const
{
	if (ImGui::TreeNode(this->name))
	{
		// TODO: Add plot here.

		for (const auto& pair : this->childMap)
			pair.second->GenerateImGuiPlotTree();

		ImGui::TreePop();
	}
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