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

		void SendJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue);

		virtual void Run() override;

	protected:
		SOCKET socket;
		ThreadSafeQueue<ParseParty::JsonValue*> jsonQueue;
		std::counting_semaphore<1024> jsonQueueSemaphore;
	};
}