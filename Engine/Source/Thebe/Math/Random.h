#pragma once

#include "Thebe/Common.h"
#include <random>

namespace Thebe
{
	class THEBE_API Random
	{
	public:
		Random();
		virtual ~Random();

		/**
		 * Set the seed for the random number generator.  This is especially useful
		 * when you want random behavior, but also deterministic behavior, such as
		 * the ability to reproduce a bug in something that is supposed to behave randomly.
		 * 
		 * @param[in] seed The mersenne twister engine is seeded with this value.
		 */
		void SetSeed(int seed);

		/**
		 * Seed the random number generator using the current time.  This makes the
		 * random number generator practically unpredictable.
		 */
		void SetSeedUsingTime();

		/**
		 * Obtain a random integer in the given range, inclusive.
		 * 
		 * @param[in] min This is the smallest possible integer that can be returned.
		 * @param[in] max This is the largest possible integer that can be returned.
		 * @return A random integer in [min,max] is returned here.
		 */
		int InRange(int min, int max);

		/**
		 * Obtain a random floating-point number in the given range, inclusive.
		 * 
		 * @param[in] min This is the smallest possible float that can be returned.
		 * @param[in] max This is the largest possible float that can be returned.
		 * @return A random float in [min,max] is returned here.
		 */
		double InRange(double min, double max);

		/**
		 * Return true or false at random with a 50/50 chance.
		 */
		bool CoinFlip();

		/**
		 * Put the elements of the given array in a random order.
		 * 
		 * @param[in,out] givenArray This is the array to shuffle.
		 * @param[in] givenArraySize Pass here the size of the given array.
		 */
		template<typename T>
		void Shuffle(T* givenArray, uint32_t givenArraySize)
		{
			for (int i = 0; i < givenArraySize - 1; i++)
			{
				int j = this->InRange(i, givenArraySize - 1);
				if (i != j)
				{
					T temp = givenArray[i];
					givenArray[i] = givenArray[j];
					givenArray[j] = temp;
				}
			}
		}

	protected:
		std::random_device randomDevice;
		std::mt19937 generator;
	};
}