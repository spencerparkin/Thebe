#pragma once

#include "Thebe/Utilities/Thread.h"
#include "JsonValue.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <functional>

namespace Thebe
{
	typedef std::function<void(std::unique_ptr<ParseParty::JsonValue>&)> JsonSocketRecvFunc;

	/**
	 * 
	 */
	class THEBE_API JsonSocketReceiver : public Thread
	{
	public:
		JsonSocketReceiver(SOCKET socket);
		virtual ~JsonSocketReceiver();

		void SetRecvFunc(JsonSocketRecvFunc recvFunc);

		virtual void ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue);

		virtual void Run() override;

	protected:
		SOCKET socket;
		JsonSocketRecvFunc recvFunc;
	};
}