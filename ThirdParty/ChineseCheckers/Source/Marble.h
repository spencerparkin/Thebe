#pragma once

#include "Vector.h"
#include <memory>

namespace ChineseCheckers
{
	class Node;

	/**
	 * These are the game pieces that hop along the game board.
	 * Varients of the game of Chinese Checkers may inherit from
	 * this class to add additional characteristics to a marble.
	 */
	class Marble
	{
	public:
		enum Color
		{
			NONE,
			BLACK,
			YELLOW,
			MAGENTA,
			GREEN,
			RED,
			BLUE,
			WHITE,
			ORANGE,
			CYAN
		};

		Marble(Color color);
		virtual ~Marble();

		Color GetColor() const;
		bool GetLocation(Vector& location) const;
		void SetOccupyingNode(std::shared_ptr<Node> node);

	protected:
		Color color;
		std::weak_ptr<Node> occupyingNode;
	};
}