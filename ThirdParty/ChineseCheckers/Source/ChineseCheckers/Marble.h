#pragma once

#include "Vector.h"
#include "JsonValue.h"
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
			YELLOW,
			RED,
			BLACK,
			GREEN,
			BLUE,
			MAGENTA,
			WHITE,
			ORANGE,
			CYAN
		};

		Marble(Color color);
		virtual ~Marble();

		virtual bool ToJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue) const;
		virtual bool FromJson(const ParseParty::JsonValue* jsonValue);

		Color GetColor() const;

	protected:
		Color color;
	};
}