#pragma once

#include "Thebe/Common.h"
#include "Thebe/Reference.h"

#if defined THEBE_LOGGING
#	define THEBE_LOG(msg, ...)			do { if (Thebe::Log::Get()) Thebe::Log::Get()->Print(msg, __VA_ARGS__); } while(false)
#else
#	define THEBE_LOG(msg, ...)
#endif

namespace Thebe
{
	class LogSink;

	/**
	 * Logging is the primary means of error reporting and general
	 * communication between Thebe and the programmer.  API function
	 * will sometimes return success or failure, but to see why a
	 * function fails, you must appeal to the log.  Logging can be
	 * completely compiled out of a release build, but it can also
	 * be left in as well.
	 */
	class THEBE_API Log : public ReferenceCounted
	{
	public:
		Log();
		virtual ~Log();

		void Print(const char* msg, ...);
		bool AddSink(LogSink* sink);
		void RemoveAllSinks();

		static void Set(Log* log);
		static Log* Get();

	private:
		static Log* log;
		std::vector<Reference<LogSink>> logSinkArray;
	};

	/**
	 * This is the base class for all logging sinks.  These
	 * take the logging information and print it somewhere.
	 */
	class THEBE_API LogSink : public ReferenceCounted
	{
	public:
		LogSink();
		virtual ~LogSink();

		virtual bool Setup();
		virtual void Print(const std::string& msg) = 0;
		virtual void Flush();
	};

	/**
	 * Print logs to the console in the context of a win32 application.
	 */
	class THEBE_API LogConsoleSink : public LogSink
	{
	public:
		LogConsoleSink();
		virtual ~LogConsoleSink();

		virtual void Print(const std::string& msg) override;
	};
}