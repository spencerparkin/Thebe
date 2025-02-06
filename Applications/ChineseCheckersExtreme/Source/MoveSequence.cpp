#include "MoveSequence.h"

MoveSequence::MoveSequence()
{
}

/*virtual*/ MoveSequence::~MoveSequence()
{
}

bool MoveSequence::ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const
{
	using namespace ParseParty;

	auto nodeIndexArrayValue = new JsonArray();
	jsonValue.reset(nodeIndexArrayValue);

	for (int i = 0; i < (int)this->nodeIndexArray.size(); i++)
		nodeIndexArrayValue->PushValue(new JsonInt(this->nodeIndexArray[i]));

	return true;
}

bool MoveSequence::FromJson(const ParseParty::JsonValue* jsonValue)
{
	using namespace ParseParty;

	auto nodeIndexArrayValue = dynamic_cast<const JsonArray*>(jsonValue);
	if (!nodeIndexArrayValue)
		return false;

	this->nodeIndexArray.clear();
	for (int i = 0; i < (int)nodeIndexArrayValue->GetSize(); i++)
	{
		auto nodeIndexValue = dynamic_cast<const JsonInt*>(nodeIndexArrayValue->GetValue(i));
		if (!nodeIndexValue)
			return false;

		this->nodeIndexArray.push_back((int)nodeIndexValue->GetValue());
	}

	return true;
}

bool MoveSequence::Extend(int nodeIndex, ChineseCheckers::Graph* graph, ChineseCheckers::Marble::Color color)
{
	if (nodeIndex < 0 || nodeIndex >= (int)graph->GetNodeArray().size())
		return false;

	if (this->nodeIndexArray.size() == 0)
	{
		ChineseCheckers::Node* node = graph->GetNodeArray()[nodeIndex];
		ChineseCheckers::Marble* marble = node->GetOccupant();
		if (!marble || marble->GetColor() != color)
			return false;

		this->nodeIndexArray.push_back(nodeIndex);
		return true;
	}

	ChineseCheckers::Node* lastNode = graph->GetNodeArray()[this->nodeIndexArray[this->nodeIndexArray.size() - 1]];

	if (this->nodeIndexArray.size() == 1)
	{
		ChineseCheckers::Node* node = graph->GetNodeArray()[nodeIndex];
		if (node->IsAdjacentTo(lastNode) && !node->GetOccupant())
		{
			this->nodeIndexArray.push_back(nodeIndex);
			return true;
		}
	}

	int originalSize = (int)this->nodeIndexArray.size();

	std::map<ChineseCheckers::Node*, int> offsetMap;
	graph->MakeOffsetMap(offsetMap);

	std::vector<const ChineseCheckers::Node*> nodeStack;
	if (!lastNode->ForAllJumps(nodeStack, [&offsetMap, nodeIndex, this](ChineseCheckers::Node* node, const std::vector<const ChineseCheckers::Node*>& nodeStack) -> bool
		{
			if (nodeIndex == offsetMap.find(node)->second)
			{
				for (int i = 1; i < (int)nodeStack.size(); i++)
					this->nodeIndexArray.push_back(offsetMap.find(const_cast<ChineseCheckers::Node*>(nodeStack[i]))->second);
				this->nodeIndexArray.push_back(nodeIndex);
				return false;
			}
			return true;
		}))
	{
		if (graph->IsValidMoveSequence(this->nodeIndexArray))
			return true;
	}

	while ((int)this->nodeIndexArray.size() > originalSize)
		this->nodeIndexArray.pop_back();

	return false;
}

bool MoveSequence::MakeMove(ChineseCheckers::Graph::Move& move, ChineseCheckers::Graph* graph) const
{
	if (!graph->IsValidMoveSequence(this->nodeIndexArray))
		return false;

	move.sourceNode = graph->GetNodeArray()[this->nodeIndexArray[0]];
	move.targetNode = graph->GetNodeArray()[this->nodeIndexArray[this->nodeIndexArray.size() - 1]];
	return true;
}