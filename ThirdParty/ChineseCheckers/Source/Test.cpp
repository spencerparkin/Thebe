#include "Test.h"
#include "Generators/TraditionalGenerator.h"
#include "Graph.h"
#include "Factory.h"

using namespace ChineseCheckers;

//--------------------------------- Test ---------------------------------

Test::Test()
{
}

/*virtual*/ Test::~Test()
{
}

//--------------------------------- TwoPlayerGameTest ---------------------------------

TwoPlayerGameTest::TwoPlayerGameTest()
{
}

/*virtual*/ TwoPlayerGameTest::~TwoPlayerGameTest()
{
}

/*virtual*/ bool TwoPlayerGameTest::Perform()
{
	Factory factory;
	TraditionalGenerator generator(&factory);
	std::set<Marble::Color> participantSet{ Marble::Color::YELLOW, Marble::Color::RED };
	std::unique_ptr<Graph> graph(generator.Generate(participantSet));

	if (graph->AllMarblesAtTarget(Marble::Color::YELLOW))
		return false;

	if (graph->AllMarblesAtTarget(Marble::Color::RED))
		return false;

	Marble::Color whoseTurn = Marble::Color::YELLOW;

	int maxIterations = 512;
	int i = 0;

	while (!(graph->AllMarblesAtTarget(Marble::Color::YELLOW) || graph->AllMarblesAtTarget(Marble::Color::RED)))
	{
		if (++i > maxIterations)
			return false;

		Vector generalDirection;
		if (!graph->CalcGeneralDirection(whoseTurn, generalDirection))
			return false;

		Graph::BestMovesCollection bestMovesCollection;
		if (!graph->FindBestMoves(whoseTurn, bestMovesCollection, generalDirection))
			return false;

		if (bestMovesCollection.moveArray.size() == 0)
		{
			// TODO: May need to deviate from general direction by a few degrees in each way.
			return false;
		}

		Graph::Move move;
		if (!bestMovesCollection.GetRandom(move))
			return false;

		if (!graph->MoveMarbleUnconditionally(move))
			return false;

		if (whoseTurn == Marble::Color::YELLOW)
			whoseTurn = Marble::Color::RED;
		else
			whoseTurn = Marble::Color::YELLOW;
	}

	return true;
}