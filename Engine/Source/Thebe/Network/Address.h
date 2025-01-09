#pragma once

#include "Thebe/Common.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

namespace Thebe
{
	/**
	 * This is an helper class that encapsulates the notion of a network IP and port.
	 */
	class THEBE_API NetworkAddress
	{
	public:
		NetworkAddress();
		NetworkAddress(const NetworkAddress& address);
		virtual ~NetworkAddress();

		void operator=(const NetworkAddress& address);

		bool SetAddress(const std::string& ipAddrAndPort);
		std::string GetAddress() const;
		void SetIPAddress(const std::string& ipAddr);
		const std::string& GetIPAddress() const;
		void SetPort(uint32_t port);
		uint32_t GetPort() const;
		void GetSockAddr(sockaddr_in& addr) const;

	private:
		std::string ipAddr;
		uint32_t port;
	};
}