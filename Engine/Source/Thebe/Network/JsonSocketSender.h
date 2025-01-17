#pragma once

#include "Thebe/Utilities/Thread.h"
#include "JsonValue.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <semaphore>

namespace Thebe
{
	/**
	 *
	 */
	class THEBE_API JsonSocketSender : public Thread
	{
	public:
		JsonSocketSender(SOCKET socket);
		virtual ~JsonSocketSender();

		void SendJson(const ParseParty::JsonValue* jsonValue);

		virtual void Run() override;

	protected:
		SOCKET socket;
		ThreadSafeQueue<std::string> jsonQueue;
		std::counting_semaphore<1024> jsonQueueSemaphore;
	};
}