#pragma once

namespace ChineseCheckers
{
	/**
	 * Test this library in some way.
	 */
	class Test
	{
	public:
		Test();
		virtual ~Test();

		/**
		 * Perform the test.
		 * 
		 * @return True should be returned if and only if the test passed.
		 */
		virtual bool Perform() = 0;
	};

	/**
	 * See if we can create and simulate a 2-player game from start to finish.
	 */
	class TwoPlayerGameTest : public Test
	{
	public:
		TwoPlayerGameTest();
		virtual ~TwoPlayerGameTest();

		virtual bool Perform() override;
	};
}