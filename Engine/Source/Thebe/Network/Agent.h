#pragma once

#include "Thebe/Common.h"
#include "Thebe/Network/Address.h"
#include "Thebe/Network/Socket.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace Thebe
{
	/**
	 * This is functionality shared by clients and servers alike.
	 */
	class THEBE_API NetworkAgent
	{
	public:
		NetworkAgent();
		virtual ~NetworkAgent();

		virtual bool Setup();
		virtual void Shutdown();

		void SetAddress(const NetworkAddress& address);
		const NetworkAddress& GetAddress() const;

		void SetSocketFactory(SocketFactoryFunction socketFactory);

	protected:
		bool MakeTCPSocket(SOCKET& socket, addrinfo*& addressInfo);

		NetworkAddress address;
		SocketFactoryFunction socketFactory;
	};
}