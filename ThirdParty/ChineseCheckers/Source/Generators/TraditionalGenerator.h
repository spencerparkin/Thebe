#pragma once

#include "../GraphGenerator.h"

namespace ChineseCheckers
{
	/**
	 * Generate the game board for the traditional game of Chinese Checkers.
	 */
	class TraditionalGenerator : public GraphGenerator
	{
	public:
		TraditionalGenerator(Factory* factory);
		virtual ~TraditionalGenerator();

		virtual Graph* Generate(const std::set<Marble::Color>& participantSet) override;

		enum
		{
			NZ = 1,
			CZ,
			KZ,
			YZ,
			PZ,
			GZ,
			RZ,
			BZ
		};

		void SetRadius(double radius);

	private:
		double radius;
	};
}