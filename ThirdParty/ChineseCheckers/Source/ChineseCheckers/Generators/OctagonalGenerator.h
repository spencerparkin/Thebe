#pragma once

#include "ChineseCheckers/GraphGenerator.h"

namespace ChineseCheckers
{
	/**
	 * Generate a game board for Chinese Checkers that is octagonal in nature.
	 */
	class OctagonalGenerator : public GraphGenerator
	{
	public:
		OctagonalGenerator(Factory* factory);
		virtual ~OctagonalGenerator();

		virtual Graph* Generate(const std::set<Marble::Color>& participantSet) override;
	};
}