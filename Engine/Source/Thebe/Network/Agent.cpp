#include "Thebe/Network/Agent.h"
#include "Thebe/Log.h"

using namespace Thebe;

NetworkAgent::NetworkAgent()
{
}

/*virtual*/ NetworkAgent::~NetworkAgent()
{
}

void NetworkAgent::SetSocketFactory(SocketFactoryFunction socketFactory)
{
	this->socketFactory = socketFactory;
}

void NetworkAgent::SetAddress(const NetworkAddress& address)
{
	this->address = address;
}

const NetworkAddress& NetworkAgent::GetAddress() const
{
	return this->address;
}

/*virtual*/ bool NetworkAgent::Setup()
{
	if (!this->socketFactory)
	{
		THEBE_LOG("Can't setup server if no socket factory configured.");
		return false;
	}

	DWORD version = MAKEWORD(2, 2);
	WSADATA startupData;
	if (WSAStartup(version, &startupData) != 0)
	{
		THEBE_LOG("Socket API start-up failed.");
		return false;
	}

	return true;
}

/*virtual*/ void NetworkAgent::Shutdown()
{
	WSACleanup();
}

bool NetworkAgent::MakeTCPSocket(SOCKET& socket, addrinfo*& addressInfo)
{
	addrinfo hints;
	::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	char portStr[32];
	sprintf_s(portStr, sizeof(portStr), "%d", this->address.GetPort());
	addressInfo = nullptr;
	int result = ::getaddrinfo(this->address.GetIPAddress().c_str(), portStr, &hints, &addressInfo);
	if (result != 0)
	{
		THEBE_LOG("Failed to get address info.  Error: %d", result);
		return false;
	}

	if (addressInfo->ai_protocol != IPPROTO_TCP)
	{
		THEBE_LOG("TCP protocol not supported by configured address.");
		return false;
	}

	socket = ::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
	if (socket == INVALID_SOCKET)
	{
		THEBE_LOG("Failed to create socket.");
		return false;
	}

	// I'm not sure if this is really necessary.  I'm seeing that sometimes I'll send data,
	// but then it will never get received.  Maybe I'm doing something else entirely wrong.
	int flag = 1;
	result = ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
	if (result == SOCKET_ERROR)
	{
		THEBE_LOG("Couldn't set the TCP no-delay socket option.  Error: %d", WSAGetLastError());
		return false;
	}

	return true;
}