#pragma once

#include "ChineseCheckers/GraphGenerator.h"

namespace ChineseCheckers
{
	/**
	 * Generate a game board for Chinese Checkers that is cubic in nature.
	 */
	class CubicGenerator : public GraphGenerator
	{
	public:
		CubicGenerator(Factory* factory);
		virtual ~CubicGenerator();

		virtual Graph* Generate(const std::set<Marble::Color>& participantSet) override;
	};
}