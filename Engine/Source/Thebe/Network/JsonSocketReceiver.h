#pragma once

#include "Thebe/Utilities/Thread.h"
#include "JsonValue.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API JsonSocketReceiver : public Thread
	{
	public:
		JsonSocketReceiver(SOCKET socket);
		virtual ~JsonSocketReceiver();

		virtual void ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue);

		virtual void Run() override;

	protected:
		SOCKET socket;
	};
}