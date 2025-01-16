#pragma once

#include "Thebe/Common.h"
#include "Thebe/Network/Agent.h"
#include "JsonValue.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API NetworkClient : public NetworkAgent
	{
	public:
		NetworkClient();
		virtual ~NetworkClient();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		NetworkSocket* GetSocket();

		void SetMaxConnectionAttempts(int maxConnectionAttempts);
		void SetRetryWaitTime(int retryWaitTimeMilliseconds);
		void SetNeedsSocketRead(bool needsSocketRead);

	protected:

		Reference<NetworkSocket> clientSocket;
		int maxConnectionAttempts;
		int retryWaitTimeMilliseconds;
	};
}