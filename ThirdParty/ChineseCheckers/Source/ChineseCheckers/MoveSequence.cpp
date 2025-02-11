#include "MoveSequence.h"

using namespace ChineseCheckers;

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

bool MoveSequence::Extend(int nodeIndex, Graph* graph, Marble::Color color)
{
	if (nodeIndex < 0 || nodeIndex >= (int)graph->GetNodeArray().size())
		return false;

	Node* newNode = graph->GetNodeArray()[nodeIndex];

	if (this->nodeIndexArray.size() == 0)
	{
		std::shared_ptr<Marble> marble = newNode->GetOccupant();
		if (!marble || marble->GetColor() != color)
			return false;

		this->nodeIndexArray.push_back(nodeIndex);
		return true;
	}

	if (newNode->GetOccupant())
		return false;

	Node* lastNode = graph->GetNodeArray()[this->nodeIndexArray[this->nodeIndexArray.size() - 1]];

	if (this->nodeIndexArray.size() == 1 && newNode->IsAdjacentTo(lastNode))
	{
		this->nodeIndexArray.push_back(nodeIndex);
		return true;
	}

	int originalSize = (int)this->nodeIndexArray.size();

	// Note that we must do a BFS here (instead of a DFS), because otherwise the
	// user will not be able to author the exact sequences they desire, because
	// we might end up taking a circutuitous path rather than the shortest path
	// from the last added node to the next node.

	std::list<int> offsetList;

	if(!lastNode->ForAllJumpsBFS([nodeIndex, &offsetList, lastNode](Node* node) -> bool
		{
			if (nodeIndex != node->GetOffset())
				return true;
			
			while (node)
			{
				offsetList.push_front(node->GetOffset());
				node = node->GetParent();
				if (node == lastNode)
					break;
			}

			return false;
		}))
	{
		for (int i : offsetList)
			this->nodeIndexArray.push_back(i);

		if (graph->IsValidMoveSequence(*this))
			return true;
	}

	while ((int)this->nodeIndexArray.size() > originalSize)
		this->nodeIndexArray.pop_back();

	return false;
}

bool MoveSequence::ToMove(Graph::Move& move, Graph* graph) const
{
	if (!graph->IsValidMoveSequence(*this))
		return false;

	move.sourceNode = graph->GetNodeArray()[this->nodeIndexArray[0]];
	move.targetNode = graph->GetNodeArray()[this->nodeIndexArray[this->nodeIndexArray.size() - 1]];
	return true;
}

bool MoveSequence::FromMove(const Graph::Move& move, Graph* graph)
{
	this->nodeIndexArray.clear();

	if (move.sourceNode->IsAdjacentTo(move.targetNode))
	{
		this->nodeIndexArray.push_back(move.sourceNode->GetOffset());
		this->nodeIndexArray.push_back(move.targetNode->GetOffset());
	}
	else
	{
		std::vector<const Node*> nodeStack;
		move.sourceNode->ForAllJumpsDFS(nodeStack, [&move, &nodeStack, this](Node* node) -> bool
			{
				if (move.targetNode != node)
					return true;

				for (const Node* intermediateNode : nodeStack)
					this->nodeIndexArray.push_back(intermediateNode->GetOffset());

				this->nodeIndexArray.push_back(node->GetOffset());
				return false;
			});
	}

	return graph->IsValidMoveSequence(*this);
}