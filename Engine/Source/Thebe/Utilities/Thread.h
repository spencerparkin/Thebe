#pragma once

#include "Thebe/Common.h"
#include <thread>

namespace Thebe
{
	/**
	 * Thin wrapper around standard threading API.
	 */
	class THEBE_API Thread
	{
	public:
		Thread();
		virtual ~Thread();

		/**
		 * Override this with any initialization needed before calling this base method to kick-off the thread.
		 * 
		 * @return True is returned if and only if we succeed.  Failsure can occur if the thread is already running.
		 */
		virtual bool Split();

		/**
		 * This should get overridden to signal the thread to shutdown before calling this base method.
		 * 
		 * @return True is returned if and only if we succeed.  Failsure can occur if the thread is not already running.
		 */
		virtual bool Join();

		/**
		 * Tell the caller if the thread is still running.  Note that it is good practice to
		 * still call the @ref Join method anyway to bring things full circle.
		 */
		bool IsRunning() const;

	protected:

		/**
		 * Override this method to perform work on the thread.  Be sure to use
		 * thread synchronization techniques to protect shared resources between threads.
		 */
		virtual void Run() = 0;

		std::thread* thread;
		volatile bool isRunning;
	};
}