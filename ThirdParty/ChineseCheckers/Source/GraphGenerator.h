#pragma once

#include "Marble.h"
#include <set>

namespace ChineseCheckers
{
	class Factory;
	class Graph;

	/**
	 * These are used to generate the structure of the game board.
	 */
	class GraphGenerator
	{
	public:
		GraphGenerator(Factory* factory);
		virtual ~GraphGenerator();

		virtual Graph* Generate(const std::set<Marble::Color>& participantSet) = 0;

	protected:
		Factory* factory;
	};
}