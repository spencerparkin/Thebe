#pragma once

#include "Thebe/Common.h"
#include "Thebe/Network/Agent.h"
#include "Thebe/Utilities/Thread.h"
#include "JsonValue.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API NetworkServer : protected Thread, public NetworkAgent
	{
	public:
		NetworkServer();
		virtual ~NetworkServer();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void SetMaxConnections(int maxConnections);

	protected:
		virtual void Run() override;

		void RemoveStaleClients();

		std::list<NetworkSocket*> connectedClientList;
		SOCKET socket;
		int maxConnections;
	};
}