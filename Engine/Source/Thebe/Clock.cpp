#include "Clock.h"
#include <Windows.h>
#include <sysinfoapi.h>

using namespace Thebe;

Clock::Clock()
{
	this->timeBase = 0;
}

/*virtual*/ Clock::~Clock()
{
}

void Clock::Reset()
{
	this->timeBase = this->GetCurrentSystemTime();
}

bool Clock::NeverBeenReset() const
{
	return this->timeBase == 0;
}

uint64_t Clock::GetCurrentSystemTime() const
{
	FILETIME fileTime{};
	::GetSystemTimePreciseAsFileTime(&fileTime);

	ULARGE_INTEGER largeInteger{};
	largeInteger.LowPart = fileTime.dwLowDateTime;
	largeInteger.HighPart = fileTime.dwHighDateTime;

	return largeInteger.QuadPart;
}

uint64_t Clock::GetElapsedTime(bool reset /*= false*/)
{
	uint64_t timeCurrent = this->GetCurrentSystemTime();
	uint64_t timeElapsed = timeCurrent - this->timeBase;
	if (reset)
		this->timeBase = timeCurrent;
	return timeElapsed;
}

double Clock::GetCurrentTimeMilliseconds(bool reset /*= false*/)
{
	long double oneHundredNanoSecondTicks = (long double)this->GetElapsedTime(reset);
	long double milliseconds = oneHundredNanoSecondTicks / 10000.0;
	return double(milliseconds);
}

double Clock::GetCurrentTimeSeconds(bool reset /*= false*/)
{
	long double oneHundredNanoSecondTicks = (long double)this->GetElapsedTime(reset);
	long double seconds = oneHundredNanoSecondTicks / 10000000.0;
	return double(seconds);
}