#include "Thebe/Network/Address.h"
#include <format>

using namespace Thebe;

//----------------------------- NetworkAddress -----------------------------

NetworkAddress::NetworkAddress()
{
	this->ipAddr = "127.0.0.1";
	this->port = 0;
}

NetworkAddress::NetworkAddress(const NetworkAddress& address)
{
	this->ipAddr = address.ipAddr;
	this->port = address.port;
}

/*virtual*/ NetworkAddress::~NetworkAddress()
{
}

void NetworkAddress::operator=(const NetworkAddress& address)
{
	this->ipAddr = address.ipAddr;
	this->port = address.port;
}

bool NetworkAddress::SetAddress(const std::string& ipAddrAndPort)
{
	if (ipAddrAndPort.length() > 0 && ::isdigit(ipAddrAndPort.c_str()[0]))
	{
		int colonPos = ipAddrAndPort.find(':');
		if (colonPos == std::string::npos)
		{
			this->ipAddr = ipAddrAndPort;
			this->port = 0;
		}
		else
		{
			this->ipAddr = ipAddrAndPort.substr(0, colonPos);
			this->port = ::atoi(ipAddrAndPort.substr(colonPos + 1).c_str());
		}
	}
	else
	{
		char hostNameBuffer[128];
		const char* hostNamePtr = nullptr;
		if (!ipAddrAndPort.empty())
			hostNamePtr = ipAddrAndPort.c_str();
		else
		{
			if (::gethostname(hostNameBuffer, sizeof(hostNameBuffer)) == SOCKET_ERROR)
				return false;

			hostNamePtr = hostNameBuffer;
		}

		hostent* host = ::gethostbyname(hostNamePtr);
		if (!host)
			return false;

		this->ipAddr = std::string(::inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
		this->port = 0;
	}

	return true;
}

std::string NetworkAddress::GetAddress() const
{
	return std::format("{}:{}", this->ipAddr.c_str(), this->port);
}

void NetworkAddress::SetIPAddress(const std::string& ipAddr)
{
	this->ipAddr = ipAddr;
}

const std::string& NetworkAddress::GetIPAddress() const
{
	return this->ipAddr;
}

void NetworkAddress::SetPort(uint32_t port)
{
	this->port = port;
}

uint32_t NetworkAddress::GetPort() const
{
	return this->port;
}

void NetworkAddress::GetSockAddr(sockaddr_in& addr) const
{
	::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = ::inet_addr(this->ipAddr.c_str());
	addr.sin_port = htons(this->port);
}