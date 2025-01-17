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

/*virtual*/ bool NetworkServer::Serve()
{
	return true;
}

/*virtual*/ void NetworkServer::Run()
{
	while (this->Serve())
	{
		this->RemoveUnconnectedClients();

		timeval timeout{};
		timeout.tv_usec = 100;

		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(this->socket, &readSet);
		int result = ::select(0, &readSet, nullptr, nullptr, &timeout);
		if (result == SOCKET_ERROR)
			break;

		if (FD_ISSET(this->socket, &readSet))
		{
			SOCKET connectedSocket = ::accept(this->socket, nullptr, nullptr);
			if (connectedSocket == INVALID_SOCKET)
				break;

			Reference<NetworkSocket> client = this->socketFactory(connectedSocket);
			THEBE_ASSERT_FATAL(client != nullptr);
			if (client->Setup())
			{
				this->connectedClientList.push_back(client);
				this->OnClientAdded(client);
			}
		}
	}

	for (NetworkSocket* client : this->connectedClientList)
		client->Shutdown();

	while (this->connectedClientList.size() > 0)
		this->RemoveUnconnectedClients();
}

/*virtual*/ void NetworkServer::OnClientAdded(NetworkSocket* networkSocket)
{
}

/*virtual*/ void NetworkServer::OnClientRemoved(NetworkSocket* networkSocket)
{
}

void NetworkServer::RemoveUnconnectedClients()
{
	std::vector<std::list<Reference<NetworkSocket>>::iterator> unconnectedClientArray;

	for (std::list<Reference<NetworkSocket>>::iterator iter = this->connectedClientList.begin(); iter != this->connectedClientList.end(); iter++)
	{
		auto client = *iter;
		if (!client->IsReaderRunning())
		{
			client->Shutdown();
			this->OnClientRemoved(client);
			unconnectedClientArray.push_back(iter);
		}
	}

	for (auto iter : unconnectedClientArray)
		this->connectedClientList.erase(iter);
}