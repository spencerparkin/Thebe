#include "Thebe/Network/Client.h"
#include "Thebe/Log.h"

using namespace Thebe;

NetworkClient::NetworkClient()
{
	this->clientSocket = nullptr;
	this->maxConnectionAttempts = 10;
	this->retryWaitTimeMilliseconds = 200;
	this->needsSocketRead = true;
}

/*virtual*/ NetworkClient::~NetworkClient()
{
	THEBE_ASSERT(this->clientSocket == nullptr);
}

void NetworkClient::SetMaxConnectionAttempts(int maxConnectionAttempts)
{
	this->maxConnectionAttempts = maxConnectionAttempts;
}

void NetworkClient::SetRetryWaitTime(int retryWaitTimeMilliseconds)
{
	this->retryWaitTimeMilliseconds = retryWaitTimeMilliseconds;
}

void NetworkClient::SetNeedsSocketRead(bool needsSocketRead)
{
	this->needsSocketRead = needsSocketRead;
}

NetworkSocket* NetworkClient::GetSocket()
{
	return this->clientSocket;
}

/*virtual*/ bool NetworkClient::Setup()
{
	if (this->clientSocket)
	{
		THEBE_LOG("Client already setup.");
		return false;
	}

	if (!NetworkAgent::Setup())
		return false;

	SOCKET socket = INVALID_SOCKET;
	addrinfo* addressInfo = nullptr;
	if (!this->MakeTCPSocket(socket, addressInfo))
		return false;

	bool connected = false;
	for (int i = 0; i < this->maxConnectionAttempts; i++)
	{
		THEBE_LOG("Connection attempt #%d...", i + 1);
		int result = ::connect(socket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
		if (result != SOCKET_ERROR)
		{
			THEBE_LOG("Connected!");
			connected = true;
			break;
		}

		::Sleep(this->retryWaitTimeMilliseconds);
	}

	freeaddrinfo(addressInfo);

	if (!connected)
	{
		THEBE_LOG("Failed to connect!");
		::closesocket(socket);
		return false;
	}

	this->clientSocket = this->socketFactory(socket);
	if (!this->clientSocket)
	{
		THEBE_LOG("Failed to make client socket.");
		return false;
	}

	if (this->needsSocketRead && !this->clientSocket->Split())
	{
		THEBE_LOG("Failed to kick-off client thread.");
		return false;
	}

	return true;
}

/*virtual*/ void NetworkClient::Shutdown()
{
	if (this->clientSocket)
	{
		this->clientSocket->Join();
		delete this->clientSocket;
		this->clientSocket = nullptr;
	}

	NetworkAgent::Shutdown();
}