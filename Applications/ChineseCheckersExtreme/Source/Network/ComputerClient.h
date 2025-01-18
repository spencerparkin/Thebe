#pragma once

#include "GameClient.h"
#include "Thebe/Utilities/Thread.h"
#include <semaphore>

/**
 * 
 */
class ComputerClient : public ChineseCheckersClient
{
public:
	ComputerClient();
	virtual ~ComputerClient();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual void Update() override;

private:

	class Brain : public Thebe::Thread
	{
	public:
		Brain(ComputerClient* computer);
		virtual ~Brain();

		virtual bool Join() override;
		virtual void Run() override;

		void FormulateTurn();

		enum Mandate
		{
			FORMULATE_TURN,
			EXIT_THREAD
		};

		Thebe::ThreadSafeQueue<Mandate> mandateQueue;
		std::binary_semaphore mandateQueueSemaphore;

		Thebe::ThreadSafeQueue<std::vector<ChineseCheckersGame::Node*>*> nodePathArrayQueue;

		ComputerClient* computer;
	};

	Brain brain;
};