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
	int colonPos = ipAddrAndPort.find(':');
	if (colonPos == std::string::npos)
		return false;
	//...
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