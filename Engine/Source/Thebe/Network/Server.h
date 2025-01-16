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

		/**
		 * This happens when a client connects to the server.
		 */
		virtual void OnClientAdded(NetworkSocket* networkSocket);

		/**
		 * This happens when the server detects that a client is no longer connected,
		 * which is not necessary at the time that the client disconnects.
		 */
		virtual void OnClientRemoved(NetworkSocket* networkSocket);

		void RemoveUnconnectedClients();

		std::list<Reference<NetworkSocket>> connectedClientList;
		SOCKET socket;
		int maxConnections;
	};
}