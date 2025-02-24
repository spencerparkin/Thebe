#pragma once

#include "ChineseCheckers/GraphGenerator.h"

namespace ChineseCheckers
{
	/**
	 * Generate a game board for Chinese Checkers that is cubic and donut-like in nature.
	 */
	class CubicDonutGenerator : public GraphGenerator
	{
	public:
		CubicDonutGenerator(Factory* factory);
		virtual ~CubicDonutGenerator();

		virtual Graph* Generate(const std::set<Marble::Color>& participantSet) override;

		bool addDiagonalConnections;
	};
}