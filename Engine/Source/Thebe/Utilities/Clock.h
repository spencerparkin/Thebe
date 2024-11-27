#pragma once

#include "Thebe/Common.h"
#include <stdint.h>

namespace Thebe
{
	class THEBE_API Clock
	{
	public:
		Clock();
		virtual ~Clock();

		bool NeverBeenReset() const;
		void Reset();

		double GetCurrentTimeMilliseconds(bool reset = false);
		double GetCurrentTimeSeconds(bool reset = false);

	private:
		uint64_t GetCurrentSystemTime() const;
		uint64_t GetElapsedTime(bool reset = false);

		uint64_t timeBase;
	};
}