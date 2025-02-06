#include "ComputerClient.h"

//----------------------------------- ComputerClient -----------------------------------

ComputerClient::ComputerClient() : brain(this)
{
	this->state = State::WAITING_FOR_TURN;
}

/*virtual*/ ComputerClient::~ComputerClient()
{
}

/*virtual*/ bool ComputerClient::Setup()
{
	this->brain.SetName("Computer Client Brain Thread");

	if (!this->brain.Split())
		return false;

	return ChineseCheckersGameClient::Setup();
}

/*virtual*/ void ComputerClient::Shutdown()
{
	this->brain.Join();

	ChineseCheckersGameClient::Shutdown();
}

/*virtual*/ void ComputerClient::Update(double deltaTimeSeconds)
{
	ChineseCheckersGameClient::Update(deltaTimeSeconds);

	switch (this->state)
	{
		case State::WAITING_FOR_TURN:
		{
			if (this->whoseTurn == this->color && this->graph.get() && this->color != ChineseCheckers::Marble::Color::NONE)
			{
				this->brain.mandateQueue.Add(Brain::Mandate::FORMULATE_TURN);
				this->brain.mandateQueueSemaphore.release();
				this->state = State::TAKING_TURN;
			}
			break;
		}
		case State::TAKING_TURN:
		{
			ChineseCheckers::MoveSequence* moveSequence = nullptr;
			if (this->brain.moveSequenceQueue.Remove(moveSequence))
			{
				this->MakeMove(*moveSequence);
				delete moveSequence;
				this->state = State::WAITING_FOR_TURN_TO_CHANGE;
			}
			break;
		}
		case State::WAITING_FOR_TURN_TO_CHANGE:
		{
			if (this->whoseTurn != this->color)
			{
				this->state = State::WAITING_FOR_TURN;
			}
			break;
		}
		default:
		{
			THEBE_ASSERT(false);
			break;
		}
	}
}

//----------------------------------- ComputerClient::Brain -----------------------------------

ComputerClient::Brain::Brain(ComputerClient* computerClient) : mandateQueueSemaphore(0)
{
	this->computerClient = computerClient;
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

bool ComputerClient::Brain::FormulateTurn()
{
	std::unique_ptr<ChineseCheckers::MoveSequence> moveSequence(new ChineseCheckers::MoveSequence());

	ChineseCheckers::Graph* graph = this->computerClient->GetGraph();	// TODO: Should probably be given clone for thread-safety.
	if (!graph)
		return false;

	ChineseCheckers::Marble::Color color = this->computerClient->GetColor();

	ChineseCheckers::Vector generalDirection;
	if (!graph->CalcGeneralDirection(color, generalDirection))
		return false;

	ChineseCheckers::Graph::BestMovesCollection bestMovesCollection;
	if (!graph->FindBestMoves(color, bestMovesCollection, generalDirection))
		return false;

	if (bestMovesCollection.moveArray.size() == 0)
		return false;

	int i = random.InRange(0, (int)bestMovesCollection.moveArray.size() - 1);
	if (!moveSequence->FromMove(bestMovesCollection.moveArray[i], graph))
		return false;

	this->moveSequenceQueue.Add(moveSequence.release());
	return true;
}