#include "Thebe/Utilities/Thread.h"

using namespace Thebe;

Thread::Thread()
{
	this->thread = nullptr;
	this->isRunning = false;
}

/*virtual*/ Thread::~Thread()
{
	THEBE_ASSERT(this->thread == nullptr);
}

/*virtual*/ bool Thread::Split()
{
	if (this->thread)
		return false;

	this->isRunning = true;
	this->thread = new std::thread([=]() {
			this->Run();
			this->isRunning = false;
		});
	return true;
}

/*virtual*/ bool Thread::Join()
{
	if (!this->thread)
		return false;

	this->thread->join();
	delete this->thread;
	this->thread = nullptr;
	return true;
}

bool Thread::IsRunning() const
{
	return this->isRunning;
}