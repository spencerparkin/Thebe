#pragma once

#include "Marble.h"
#include "Vector.h"

namespace ChineseCheckers
{
	class Graph;
	class Node;
	class Marble;

	/**
	 * Generators take a factory that they can use to generate the initial
	 * state of a game.  The factory can be inherited so as to customize
	 * the elements used in the game.
	 */
	class Factory
	{
	public:
		Factory();
		virtual ~Factory();

		virtual Graph* CreateGraph();
		virtual Node* CreateNode(const Vector& location, Marble::Color color);
		virtual Marble* CreateMarble(Marble::Color color);
	};
}