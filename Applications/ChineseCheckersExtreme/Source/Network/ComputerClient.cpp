#include "ComputerClient.h"

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
	/*
	std::unique_ptr<std::vector<ChineseCheckersGame::Node*>> nodePathArray(new std::vector<ChineseCheckersGame::Node*>());

	for (const ChineseCheckersGame::Occupant* occupant : this->computer->GetGame()->GetOccupantArray())
	{

	}

	//std::vector<ChineseCheckersGame::Node*> currentNodePathArray;

	this->nodePathArrayQueue.Add(nodePathArray.release());
	*/
}