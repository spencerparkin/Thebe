#pragma once

#include "GameClient.h"
#include "Thebe/Utilities/Thread.h"
#include "Thebe/Math/Random.h"
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
	virtual void Update(double deltaTimeSeconds) override;

private:
	class Brain : public Thebe::Thread
	{
	public:
		Brain(ComputerClient* computer);
		virtual ~Brain();

		virtual bool Join() override;
		virtual void Run() override;

		void FormulateTurn();
		void SearchPathsRecursive(
			ChineseCheckersGame::Node* node,
			std::vector<ChineseCheckersGame::Node*>& nodePathArray,
			bool canRecurse,
			std::vector<double>& scoreStack,
			std::vector<ChineseCheckersGame::Node*>* bestNodePath,
			double& bestScore);

		enum Mandate
		{
			FORMULATE_TURN,
			EXIT_THREAD
		};

		Thebe::ThreadSafeQueue<Mandate> mandateQueue;
		std::binary_semaphore mandateQueueSemaphore;

		Thebe::ThreadSafeQueue<std::vector<ChineseCheckersGame::Node*>*> nodePathArrayQueue;

		ComputerClient* computer;

		class IdealDirectionSet : public Thebe::ReferenceCounted
		{
		public:
			void GoDirection(int i, std::vector<double>& scoreStack) const;

			std::set<int> directionSet;
		};

		std::map<Thebe::RefHandle, Thebe::Reference<IdealDirectionSet>> idealDirectionMap;

		void GenerateIdealDirectionMap(int targetZoneID);
		void GoDirection(ChineseCheckersGame::Node* node, int i, std::vector<double>& scoreStack);

		Thebe::Random random;
	};

	Brain brain;

	enum State
	{
		WAITING_FOR_MY_TURN,
		THINKING,
		WAITING_FOR_TURN_TO_HAPPEN,
		CHILL
	};

	State state;
	double chillTimeSeconds;
};