#include "Thebe/Utilities/Thread.h"
#include <Windows.h>
#include <processthreadsapi.h>
#include <locale>
#include <codecvt>

using namespace Thebe;

Thread::Thread(const std::string& name /*= "Unnamed Thread"*/)
{
	this->name = name;
	this->thread = nullptr;
	this->isRunning = false;
}

/*virtual*/ Thread::~Thread()
{
	THEBE_ASSERT(this->thread == nullptr);
}

void Thread::SetName(const std::string& name)
{
	this->name = name;
}

/*virtual*/ bool Thread::Split()
{
	if (this->thread)
		return false;

	this->isRunning = true;
	this->thread = new std::thread([=]()
		{
			HANDLE threadHandle = GetCurrentThread();
			std::wstring nameWide = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->name);
			::SetThreadDescription(threadHandle, nameWide.c_str());
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