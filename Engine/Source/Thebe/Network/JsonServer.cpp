#include "Thebe/Network/JsonServer.h"

using namespace Thebe;

//---------------------------------- JsonServer ----------------------------------

JsonServer::JsonServer()
{
	this->socket = INVALID_SOCKET;
	this->clientManager = nullptr;
}

/*virtual*/ JsonServer::~JsonServer()
{
	THEBE_ASSERT(this->clientManager == nullptr);
}

void JsonServer::SetAddress(const NetworkAddress& address)
{
	this->address = address;
}

/*virtual*/ bool JsonServer::Setup()
{
	if (this->socket != INVALID_SOCKET)
		return false;

	DWORD version = MAKEWORD(2, 2);
	WSADATA startupData;
	if (WSAStartup(version, &startupData) != 0)
		return false;

	addrinfo hints;
	::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	char portStr[32];
	sprintf_s(portStr, sizeof(portStr), "%d", this->address.GetPort());
	addrinfo* addressInfo = nullptr;
	int result = ::getaddrinfo(this->address.GetIPAddress().c_str(), portStr, &hints, &addressInfo);
	if (result != 0)
		return false;

	if (addressInfo->ai_protocol != IPPROTO_TCP)
		return false;

	socket = ::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
	if (socket == INVALID_SOCKET)
		return false;

	this->clientManager = new ClientManagerThread(this);
	if (!this->clientManager->Split())
		return false;

	return true;
}

/*virtual*/ void JsonServer::Shutdown()
{
	if (this->socket != INVALID_SOCKET)
	{
		::closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	if (this->clientManager)
	{
		this->clientManager->Join();
		delete this->clientManager;
		this->clientManager = nullptr;
	}

	WSACleanup();
}

/*virtual*/ void JsonServer::Serve()
{
	ClientMessage message;
	if (this->clientMessageQueue.Remove(message))
	{
		this->ProcessClientMessage(&message);
		delete message.jsonValue;
	}
}

SOCKET JsonServer::GetSocket()
{
	return this->socket;
}

/*virtual*/ void JsonServer::ProcessClientMessage(ClientMessage* message)
{
}

/*virtual*/ void JsonServer::OnClientConnected(ConnectedClient* client)
{
}

/*virtual*/ void JsonServer::OnClientDisconnected(ConnectedClient* client)
{
}

//---------------------------------- JsonServer::ClientManagerThread ----------------------------------

JsonServer::ClientManagerThread::ClientManagerThread(JsonServer* server)
{
	this->server = server;
}

/*virtual*/ JsonServer::ClientManagerThread::~ClientManagerThread()
{
}

/*virtual*/ void JsonServer::ClientManagerThread::Run()
{
	while (true)
	{
		SOCKET socket = this->server->GetSocket();

		timeval timeout{};
		timeout.tv_sec = 1.0;

		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(socket, &readSet);
		int result = ::select(socket, &readSet, nullptr, nullptr, &timeout);
		if (result == SOCKET_ERROR)
			break;

		if (FD_ISSET(socket, &readSet))
		{
			SOCKET connectedSocket = ::accept(socket, nullptr, nullptr);
			if (connectedSocket == INVALID_SOCKET)
				break;

			Reference<ConnectedClient> client(new ConnectedClient(connectedSocket, this->server));
			if (!client->Setup())
				client->Shutdown();
			else
			{
				std::lock_guard lock(this->connectedClientListMutex);
				this->connectedClientList.push_back(client);
				this->server->OnClientConnected(client);
			}
		}
		else
		{
			std::lock_guard lock(this->connectedClientListMutex);

			std::vector<std::list<Reference<ConnectedClient>>::iterator> staleClientArray;
			for (std::list<Reference<ConnectedClient>>::iterator iter = this->connectedClientList.begin(); iter != this->connectedClientList.end(); iter++)
			{
				auto& client = *iter;
				if (!client->StillConnected())
				{
					staleClientArray.push_back(iter);
					this->server->OnClientDisconnected(client);
				}
			}

			for (auto& iter : staleClientArray)
				this->connectedClientList.erase(iter);
		}
	}

	// Shutdown all the connected clients before we exit the thread.
	{
		std::lock_guard lock(this->connectedClientListMutex);

		for (auto& client : this->connectedClientList)
			client->Shutdown();

		this->connectedClientList.clear();
	}
}

void JsonServer::ClientManagerThread::SendJsonToAllClients(const ParseParty::JsonValue* jsonValue)
{
	std::lock_guard lock(this->connectedClientListMutex);
	for (auto& client : this->connectedClientList)
		client->SendJson(jsonValue);
}

//---------------------------------- JsonServer::ConnectedClient ----------------------------------

JsonServer::ConnectedClient::ConnectedClient(SOCKET connectedSocket, JsonServer* server)
{
	this->connectedSocket = connectedSocket;
	this->server = server;
	this->sender = nullptr;
	this->receiver = nullptr;
	this->userData = 0;
}

/*virtual*/ JsonServer::ConnectedClient::~ConnectedClient()
{
	THEBE_ASSERT(this->sender == nullptr);
	THEBE_ASSERT(this->receiver = nullptr);
}

bool JsonServer::ConnectedClient::Setup()
{
	if (this->sender || this->receiver)
		return false;

	this->sender = new JsonSocketSender(this->connectedSocket);
	if (!this->sender->Split())
		return false;

	this->receiver = new JsonSocketReceiver(this->connectedSocket);
	this->receiver->SetRecvFunc([=](std::unique_ptr<ParseParty::JsonValue>& jsonValue) { this->ReceiveJson(jsonValue.release()); });
	if (!this->receiver->Split())
		return false;

	return true;
}

void JsonServer::ConnectedClient::Shutdown()
{
	if (this->sender)
	{
		this->sender->Join();
		delete this->sender;
		this->sender = nullptr;
	}

	if (this->receiver)
	{
		this->receiver->Join();
		delete this->receiver;
		this->receiver = nullptr;
	}
}

bool JsonServer::ConnectedClient::StillConnected()
{
	if (this->receiver && !this->receiver->IsRunning())
		return false;

	if (this->sender && !this->sender->IsRunning())
		return false;

	return true;
}

void JsonServer::ConnectedClient::ReceiveJson(const ParseParty::JsonValue* jsonValue)
{
	ClientMessage message;
	message.jsonValue = jsonValue;
	message.client = this;
	this->server->clientMessageQueue.Add(message);
}

void JsonServer::ConnectedClient::SendJson(const ParseParty::JsonValue* jsonValue)
{
	if (this->sender)
		this->sender->SendJson(jsonValue);
}

uintptr_t JsonServer::ConnectedClient::GetUserData()
{
	return this->userData;
}

void JsonServer::ConnectedClient::SetUserData(uintptr_t userData)
{
	this->userData = userData;
}