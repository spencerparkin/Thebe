#include "Log.h"
#include <stdarg.h>
#include <ctime>
#include <format>
#include <Windows.h>

namespace Thebe
{
	Log* Log::log = nullptr;
}

using namespace Thebe;

//------------------------------------ Log ------------------------------------

Log::Log()
{
}

/*virtual*/ Log::~Log()
{
}

void Log::Print(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	char formattedMessageBuffer[1024];
	vsprintf_s(formattedMessageBuffer, sizeof(formattedMessageBuffer), msg, args);

	std::time_t time = std::time(nullptr);
	char timeBuffer[128];
	std::strftime(timeBuffer, sizeof(timeBuffer), "%T", std::localtime(&time));
	std::string formattedMessage = std::format("{}: {}\n", timeBuffer, formattedMessageBuffer);
	
	for (LogSink* logSink : this->logSinkArray)
		logSink->Print(formattedMessage);

	va_end(args);
}

bool Log::AddSink(LogSink* sink)
{
	if (!sink->Setup())
		return false;

	this->logSinkArray.push_back(sink);
	return true;
}

void Log::RemoveAllSinks()
{
	this->logSinkArray.clear();
}

/*static*/ void Log::Set(Log* log)
{
	Log::log = log;
}

/*static*/ Log* Log::Get()
{
	return Log::log;
}

//------------------------------------ LogSink ------------------------------------

LogSink::LogSink()
{
}

/*virtual*/ LogSink::~LogSink()
{
}

/*virtual*/ bool LogSink::Setup()
{
	return true;
}

/*virtual*/ void LogSink::Flush()
{
}

//------------------------------------ LogConsoleSink ------------------------------------

LogConsoleSink::LogConsoleSink()
{
}

/*virtual*/ LogConsoleSink::~LogConsoleSink()
{
}

/*virtual*/ void LogConsoleSink::Print(const std::string& msg)
{
	OutputDebugStringA(msg.c_str());
}