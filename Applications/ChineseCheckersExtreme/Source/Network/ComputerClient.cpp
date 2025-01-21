#include "ComputerClient.h"

using namespace Thebe;

//------------------------------------ ComputerClient ------------------------------------

ComputerClient::ComputerClient() : brain(this)
{
}

/*virtual*/ ComputerClient::~ComputerClient()
{
}

/*virtual*/ bool ComputerClient::Setup()
{
	if (!this->brain.Split())
		return false;

	return ChineseCheckersClient::Setup();
}

/*virtual*/ void ComputerClient::Shutdown()
{
	this->brain.Join();

	ChineseCheckersClient::Shutdown();
}

/*virtual*/ void ComputerClient::Update()
{
	ChineseCheckersClient::Update();

	if (this->whoseTurnZoneID == this->GetSourceZoneID())
	{
		this->brain.mandateQueue.Add(Brain::Mandate::FORMULATE_TURN);
		this->brain.mandateQueueSemaphore.release();
	}

	std::vector<ChineseCheckersGame::Node*>* nodePathArray = nullptr;
	if (this->brain.nodePathArrayQueue.Remove(nodePathArray))
	{
		this->TakeTurn(*nodePathArray);
		delete nodePathArray;
	}
}

//------------------------------------ ComputerClient::Brain ------------------------------------

ComputerClient::Brain::Brain(ComputerClient* computer) : mandateQueueSemaphore(0)
{
	this->computer = computer;
}

/*virtual*/ ComputerClient::Brain::~Brain()
{
}

/*virtual*/ bool ComputerClient::Brain::Join()
{
	this->mandateQueue.Add(EXIT_THREAD);
	this->mandateQueueSemaphore.release();

	return Thread::Join();
}

/*virtual*/ void ComputerClient::Brain::Run()
{
	while (true)
	{
		this->mandateQueueSemaphore.acquire();

		Mandate mandate = EXIT_THREAD;
		if (!this->mandateQueue.Remove(mandate))
			break;

		if (mandate == EXIT_THREAD)
			break;

		if (mandate == FORMULATE_TURN)
			this->FormulateTurn();
	}
}

void ComputerClient::Brain::FormulateTurn()
{
	// This AI is dumb, but my goal here for now is to just make something that works as an automated opponent.

	std::unique_ptr<std::vector<ChineseCheckersGame::Node*>> bestNodePathArray(new std::vector<ChineseCheckersGame::Node*>());

	ChineseCheckersGame* game = this->computer->GetGame();

	for (const Reference<ChineseCheckersGame::Node>& node : game->GetNodeArray())
	{
		if (!node->occupant)
			continue;

		if (node->occupant->sourceZoneID != this->computer->sourceZoneID)
			continue;

		if (this->idealDirectionMap.size() == 0)
			this->GenerateIdealDirectionMap(node->occupant->targetZoneID);

		std::vector<ChineseCheckersGame::Node*> nodePathArray;
		double bestScore = 0.0;
		std::vector<double> scoreStack;
		this->SearchPathsRecursive(const_cast<ChineseCheckersGame::Node*>(node.Get()), nodePathArray, true, scoreStack, bestNodePathArray.get(), bestScore);
	}

	this->nodePathArrayQueue.Add(bestNodePathArray.release());
}

void ComputerClient::Brain::SearchPathsRecursive(
									ChineseCheckersGame::Node* node,
									std::vector<ChineseCheckersGame::Node*>& nodePathArray,
									bool canRecurse,
									std::vector<double>& scoreStack,
									std::vector<ChineseCheckersGame::Node*>* bestNodePath,
									double bestScore)
{
	nodePathArray.push_back(node);

	double pathScore = 0.0;
	for (double score : scoreStack)
		pathScore += score;

	if (pathScore > bestScore)
	{
		bestScore = pathScore;
		*bestNodePath = nodePathArray;
	}

	if (canRecurse)
	{
		for (int i = 0; i < (int)node->adjacentNodeArray.size(); i++)
		{
			Vector3 unitDirection;
			ChineseCheckersGame::Node* adjacentNode = node->GetAdjacencyAndDirection(i, unitDirection);
			if (!adjacentNode->occupant && nodePathArray.size() == 1)
			{
				this->GoDirection(node, i, scoreStack);
				this->SearchPathsRecursive(adjacentNode, nodePathArray, false, scoreStack, bestNodePath, bestScore);
				scoreStack.pop_back();
			}

			if (adjacentNode->occupant)
			{
				int j = -1;
				ChineseCheckersGame::Node* hopNode = adjacentNode->GetAdjacencyInDirection(unitDirection, &j);
				if (hopNode && !hopNode->occupant)
				{
					bool alreadyTraveled = false;
					for (int k = 0; k < (int)nodePathArray.size() && !alreadyTraveled; k++)
						if (nodePathArray[k] == hopNode)
							alreadyTraveled = true;

					if (!alreadyTraveled)
					{
						this->GoDirection(node, i, scoreStack);
						this->GoDirection(adjacentNode, j, scoreStack);
						this->SearchPathsRecursive(hopNode, nodePathArray, true, scoreStack, bestNodePath, bestScore);
						scoreStack.pop_back();
						scoreStack.pop_back();
					}
				}
			}
		}
	}

	nodePathArray.pop_back();
}

void ComputerClient::Brain::GoDirection(ChineseCheckersGame::Node* node, int i, std::vector<double>& scoreStack)
{
	auto pair = this->idealDirectionMap.find(node->GetHandle());
	THEBE_ASSERT(pair != this->idealDirectionMap.end());
	IdealDirectionSet* idealDirectionSet = pair->second.Get();
	idealDirectionSet->GoDirection(i, scoreStack);
}

void ComputerClient::Brain::GenerateIdealDirectionMap(int targetZoneID)
{
	this->idealDirectionMap.clear();

	ChineseCheckersGame* game = this->computer->GetGame();

	std::set<RefHandle> zoneNodeSet;

	// The initial ideal directions are simply to move from a non-target zone to a target zone.
	for (const Reference<ChineseCheckersGame::Node>& node : game->GetNodeArray())
	{
		if (node->zoneID != targetZoneID)
		{
			Reference<IdealDirectionSet> idealDirectionSet(new IdealDirectionSet());

			for (int i = 0; i < (int)node->adjacentNodeArray.size(); i++)
			{
				ChineseCheckersGame::Node* adjacentNode = node->adjacentNodeArray[i];

				if (adjacentNode->zoneID == targetZoneID)
				{
					idealDirectionSet->directionSet.insert(i);
					zoneNodeSet.insert(adjacentNode->GetHandle());
				}
			}

			if (idealDirectionSet->directionSet.size() > 0)
				this->idealDirectionMap.insert(std::pair(node->GetHandle(), idealDirectionSet));
		}
	}

	// Ideal directions within the target zone are to move deeper within the zone.
	std::vector<RefHandle> additionalNodesArray;
	while (true)
	{
		additionalNodesArray.clear();

		for (const Reference<ChineseCheckersGame::Node>& node : game->GetNodeArray())
		{
			if (zoneNodeSet.find(node->GetHandle()) != zoneNodeSet.end())
			{
				Reference<IdealDirectionSet> idealDirectionSet(new IdealDirectionSet());

				for (int i = 0; i < (int)node->adjacentNodeArray.size(); i++)
				{
					ChineseCheckersGame::Node* adjacentNode = node->adjacentNodeArray[i];

					if (adjacentNode->zoneID == targetZoneID && zoneNodeSet.find(adjacentNode->GetHandle()) == zoneNodeSet.end())
					{
						idealDirectionSet->directionSet.insert(i);
						additionalNodesArray.push_back(node->GetHandle());
					}
				}

				if (idealDirectionSet->directionSet.size() > 0)
					this->idealDirectionMap.insert(std::pair(node->GetHandle(), idealDirectionSet));
			}
		}

		if (additionalNodesArray.size() == 0)
			break;

		for (auto& refHandle : additionalNodesArray)
			zoneNodeSet.insert(refHandle);
	}

	// Finally, it's ideal to move from anywhere where we don't know where to go to somewhere where we do know where to go.
	while (this->idealDirectionMap.size() < game->GetNodeArray().size())
	{
		for (const Reference<ChineseCheckersGame::Node>& node : game->GetNodeArray())
		{
			if (node->zoneID != targetZoneID)
			{
				Reference<IdealDirectionSet> idealDirectionSet(new IdealDirectionSet());

				for (int i = 0; i < (int)node->adjacentNodeArray.size(); i++)
				{
					const ChineseCheckersGame::Node* adjacentNode = node->adjacentNodeArray[i];

					if (this->idealDirectionMap.find(adjacentNode->GetHandle()) != this->idealDirectionMap.end())
						idealDirectionSet->directionSet.insert(i);
				}

				if (idealDirectionSet->directionSet.size() > 0)
					this->idealDirectionMap.insert(std::pair(node->GetHandle(), idealDirectionSet));
			}
		}
	}
}

void ComputerClient::Brain::IdealDirectionSet::GoDirection(int i, std::vector<double>& scoreStack) const
{
	double score = 0.0;
	if (this->directionSet.find(i) != this->directionSet.end())
		score = 1.0;

	scoreStack.push_back(score);
}