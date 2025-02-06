#pragma once

#include "GameClient.h"
#include "MoveSequence.h"
#include "Thebe/Utilities/Thread.h"
#include "Thebe/Math/Random.h"
#include <semaphore>

class ComputerClient : public ChineseCheckersGameClient
{
public:
	ComputerClient();
	virtual ~ComputerClient();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual void Update(double deltaTimeSeconds) override;

private:
	enum State
	{
		WAITING_FOR_TURN,
		TAKING_TURN,
		WAITING_FOR_TURN_TO_CHANGE
	};

	State state;

	class Brain : public Thebe::Thread
	{
	public:
		Brain(ComputerClient* computerClient);
		virtual ~Brain();

		virtual bool Join() override;
		virtual void Run() override;

		bool FormulateTurn();

		enum Mandate
		{
			FORMULATE_TURN,
			EXIT_THREAD
		};

		Thebe::ThreadSafeQueue<Mandate> mandateQueue;
		std::binary_semaphore mandateQueueSemaphore;

		Thebe::ThreadSafeQueue<MoveSequence*> moveSequenceQueue;

		ComputerClient* computerClient;
		Thebe::Random random;
	};

	Brain brain;
};