#include "Thebe/Network/Server.h"
#include "Thebe/Utilities/RingBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

//--------------------------------- NetworkServer ---------------------------------

NetworkServer::NetworkServer()
{
	this->socket = INVALID_SOCKET;
	this->maxConnections = 128;
}

/*virtual*/ NetworkServer::~NetworkServer()
{
	THEBE_ASSERT(this->connectedClientList.size() == 0);
}

void NetworkServer::SetMaxConnections(int maxConnections)
{
	this->maxConnections = maxConnections;
}

/*virtual*/ bool NetworkServer::Setup()
{
	if (!NetworkAgent::Setup())
		return false;

	addrinfo* addressInfo = nullptr;
	if (!this->MakeTCPSocket(this->socket, addressInfo))
		return false;

	int result = ::bind(this->socket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
	if (result == SOCKET_ERROR)
	{
		THEBE_LOG("Failed to bind socket to address.  Error: %d", WSAGetLastError());
		return false;
	}

	freeaddrinfo(addressInfo);

	result = ::listen(this->socket, this->maxConnections);
	if (result == SOCKET_ERROR)
	{
		THEBE_LOG("Server failed to listen on socket.  Error: %d", WSAGetLastError());
		return false;
	}

	if (!this->Split())
	{
		THEBE_LOG("Server failed to kick-off server thread.");
		return false;
	}

	return true;
}

/*virtual*/ void NetworkServer::Shutdown()
{
	if (this->socket != INVALID_SOCKET)
	{
		::closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	this->Join();

	NetworkAgent::Shutdown();
}

/*virtual*/ void NetworkServer::Run()
{
	while (true)
	{
		// This will block until we get a connection or we're signaled to exit this thread by a closure of the socket.
		SOCKET connectedSocket = ::accept(this->socket, nullptr, nullptr);
		if (connectedSocket == INVALID_SOCKET)
			break;

		auto client = this->socketFactory(connectedSocket);
		THEBE_ASSERT_FATAL(client != nullptr);
		if (!client->Split())
			delete client;
		else
			this->connectedClientList.push_back(client);

		// Before we go back to accepting new connections, take this opportunity
		// to clean up any clients that have exited while we were blocked.
		this->RemoveStaleClients();
	}

	for (NetworkSocket* client : this->connectedClientList)
		client->Join();

	while (this->connectedClientList.size() > 0)
		this->RemoveStaleClients();
}

void NetworkServer::RemoveStaleClients()
{
	std::vector<std::list<NetworkSocket*>::iterator> staleClientArray;

	for (std::list<NetworkSocket*>::iterator iter = this->connectedClientList.begin(); iter != this->connectedClientList.end(); iter++)
	{
		auto client = *iter;
		if (!client->IsRunning())
		{
			client->Join();
			delete client;
			staleClientArray.push_back(iter);
		}
	}

	for (auto iter : staleClientArray)
		this->connectedClientList.erase(iter);
}