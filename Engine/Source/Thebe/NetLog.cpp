#include "Thebe/NetLog.h"
#include "Thebe/Utilities/RingBuffer.h"
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

//----------------------------- NetLogCollector -----------------------------

NetLogCollector::NetLogCollector()
{
}

/*virtual*/ NetLogCollector::~NetLogCollector()
{
}

bool NetLogCollector::RemoveLogMessage(std::string& logMessage)
{
	if (this->logMessageList.size() > 0)
	{
		std::scoped_lock<std::mutex> lock(this->logMessageListMutex);
		if (this->logMessageList.size() > 0)
		{
			logMessage = *logMessageList.begin();
			logMessageList.pop_front();
			return true;
		}
	}

	return false;
}

void NetLogCollector::AddLogMessage(const std::string& logMessage)
{
	std::lock_guard<std::mutex> lock(this->logMessageListMutex);
	this->logMessageList.push_back(logMessage);
}

//----------------------------- DatagramLogSink -----------------------------

DatagramLogSink::DatagramLogSink()
{
	this->socket = INVALID_SOCKET;
}

/*virtual*/ DatagramLogSink::~DatagramLogSink()
{
	if (this->socket != INVALID_SOCKET)
		::closesocket(this->socket);

	WSACleanup();
}

/*virtual*/ bool DatagramLogSink::Setup()
{
	if (this->socket != INVALID_SOCKET)
		return false;

	DWORD version = MAKEWORD(2, 2);
	WSADATA startupData;
	if (WSAStartup(version, &startupData) != 0)
		return false;

	this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (this->socket == INVALID_SOCKET)
		return false;

	return true;
}

/*virtual*/ void DatagramLogSink::Print(const std::string& msg)
{
	if (this->socket != INVALID_SOCKET)
	{
		sockaddr_in addr;
		this->sendAddress.GetSockAddr(addr);
		int numBytesSent = ::sendto(this->socket, msg.c_str(), msg.length(), 0, (const sockaddr*)&addr, sizeof(addr));
		numBytesSent = 0;
	}
}

void DatagramLogSink::SetSendAddress(const NetworkAddress& sendAddress)
{
	this->sendAddress = sendAddress;
}

//----------------------------- DatagramLogSink -----------------------------

DatagramLogCollector::DatagramLogCollector()
{
	this->thread = nullptr;
	this->socket = INVALID_SOCKET;
}

/*virtual*/ DatagramLogCollector::~DatagramLogCollector()
{
}

void DatagramLogCollector::SetReceptionAddress(const NetworkAddress& receptionAddress)
{
	this->receptionAddress = receptionAddress;
}

const NetworkAddress& DatagramLogCollector::GetReceptionAddress() const
{
	return this->receptionAddress;
}

bool DatagramLogCollector::Setup()
{
	if (this->thread || this->socket != INVALID_SOCKET)
		return false;

	DWORD version = MAKEWORD(2, 2);
	WSADATA startupData{};
	if (WSAStartup(version, &startupData) != 0)
		return false;

	this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (this->socket == INVALID_SOCKET)
		return false;

	char flag = 1;
	int error = ::setsockopt(this->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag));
	if (error != 0)
		return false;

	sockaddr_in addr;
	this->receptionAddress.GetSockAddr(addr);
	error = ::bind(this->socket, (const sockaddr*)&addr, sizeof(addr));
	if (error < 0)
		return false;

	this->thread = new std::thread([=]() { this->ThreadRun(); });
	return true;
}

void DatagramLogCollector::Shutdown()
{
	if (this->socket != INVALID_SOCKET)
	{
		// Note that closing the socket will signal our thread to exit.
		::closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	if (this->thread)
	{
		this->thread->join();
		delete this->thread;
		this->thread = nullptr;
	}

	WSACleanup();

	this->logMessageList.clear();
}

void DatagramLogCollector::ThreadRun()
{
	uint32_t maxMessageSize = 2048;
	std::unique_ptr<uint8_t> messageBuffer(new uint8_t[maxMessageSize]);

	while (true)
	{
		int numBytesReceived = ::recvfrom(this->socket, (char*)messageBuffer.get(), (int)maxMessageSize, 0, nullptr, nullptr);
		if (numBytesReceived == 0 || numBytesReceived == SOCKET_ERROR)
			break;

		messageBuffer.get()[maxMessageSize - 1] = '\0';
		std::string logMessage((const char*)messageBuffer.get());
		this->AddLogMessage(logMessage);
	}
}

//----------------------------- NetClientLogSink -----------------------------

NetClientLogSink::NetClientLogSink()
{
	this->socket = INVALID_SOCKET;
}

/*virtual*/ NetClientLogSink::~NetClientLogSink()
{
	if (this->socket != INVALID_SOCKET)
	{
		::closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	WSACleanup();
}

/*virtual*/ bool NetClientLogSink::Setup()
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
	sprintf_s(portStr, sizeof(portStr), "%d", this->connectAddress.GetPort());
	addrinfo* info = nullptr;
	int result = getaddrinfo(this->connectAddress.GetIPAddress().c_str(), portStr, &hints, &info);
	if (result != 0)
	{
		int error = WSAGetLastError();
		return false;
	}

	if (info->ai_protocol != IPPROTO_TCP)
		return false;

	this->socket = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (this->socket == INVALID_SOCKET)
		return false;

	int numConnectionAttempts = 10;
	bool connected = false;
	for (int i = 0; i < numConnectionAttempts; i++)
	{
		result = ::connect(this->socket, info->ai_addr, (int)info->ai_addrlen);
		if (result != SOCKET_ERROR)
		{
			connected = true;
			break;
		}

		::Sleep(200);
	}

	::freeaddrinfo(info);

	if (!connected)
	{
		::closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	return connected;
}

/*virtual*/ void NetClientLogSink::Print(const std::string& msg)
{
	if (this->socket != INVALID_SOCKET)
	{
		int numBytesSent = ::send(socket, msg.c_str(), msg.length(), 0);
		numBytesSent = 0;
	}
}

void NetClientLogSink::SetConnectAddress(const NetworkAddress& connectAddress)
{
	this->connectAddress = connectAddress;
}

//----------------------------- NetClientLogSink -----------------------------

NetServerLogCollector::NetServerLogCollector()
{
	this->listenSocket = INVALID_SOCKET;
	this->listenThread = nullptr;
	
}
/*virtual*/ NetServerLogCollector::~NetServerLogCollector()
{
}

void NetServerLogCollector::SetListeningAddress(const NetworkAddress& listenAddress)
{
	this->listenAddress = listenAddress;
}

/*virtual*/ bool NetServerLogCollector::Setup()
{
	if (this->listenThread)
		return false;

	this->listenThread = new std::thread([=]() {this->ListenThreadRun(); });
	return true;
}

/*virtual*/ void NetServerLogCollector::Shutdown()
{
	if (this->listenSocket != INVALID_SOCKET)
	{
		::closesocket(this->listenSocket);
		this->listenSocket = INVALID_SOCKET;
	}

	if (this->listenThread)
	{
		this->listenThread->join();
		delete this->listenThread;
		this->listenThread = nullptr;
	}
}

void NetServerLogCollector::ListenThreadRun()
{
	DWORD version = MAKEWORD(2, 2);
	WSADATA startupData;
	if (WSAStartup(version, &startupData) != 0)
		return;

	addrinfo hints;
	::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	char portStr[32];
	sprintf_s(portStr, sizeof(portStr), "%d", this->listenAddress.GetPort());
	addrinfo* info = nullptr;
	int result = ::getaddrinfo(this->listenAddress.GetIPAddress().c_str(), portStr, &hints, &info);
	if (result != 0)
		return;

	if (info->ai_protocol != IPPROTO_TCP)
		return;

	this->listenSocket = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (this->listenSocket == INVALID_SOCKET)
		return;

	result = ::bind(this->listenSocket, info->ai_addr, (int)info->ai_addrlen);
	if (result == SOCKET_ERROR)
		return;

	freeaddrinfo(info);

	result = ::listen(this->listenSocket, 128);
	if (result == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		error = 0;
		return;
	}

	while (true)
	{
		SOCKET connectedSocket = ::accept(this->listenSocket, nullptr, nullptr);
		if (connectedSocket == INVALID_SOCKET)
			break;

		auto client = new Client();
		client->socket = connectedSocket;
		client->exited = false;
		client->thread = new std::thread([=]() { this->ClientThreadRun(client); });
		this->clientList.push_back(client);

		this->RemoveExitedClients();
	}

	for (Client* client : this->clientList)
		::closesocket(client->socket);

	while (this->clientList.size() > 0)
		this->RemoveExitedClients();
}

void NetServerLogCollector::RemoveExitedClients()
{
	std::list<Client*>::iterator iter = this->clientList.begin();
	while (iter != this->clientList.end())
	{
		std::list<Client*>::iterator nextIter = iter;
		nextIter++;

		Client* client = *iter;
		if (client->exited)
		{
			client->thread->join();
			delete client->thread;
			delete client;
			this->clientList.erase(iter);
		}

		iter = nextIter;
	}
}

void NetServerLogCollector::ClientThreadRun(Client* client)
{
	RingBuffer ringBuffer(2048);
	std::vector<char> byteArray;

	while (true)
	{
		char buffer[128];
		int numBytes = ::recv(client->socket, buffer, sizeof(buffer), 0);
		if (numBytes <= 0)
			break;

		if (!ringBuffer.WriteBytes((uint8_t*)buffer, numBytes))
			break;

		uint32_t numBytesStored = ringBuffer.GetNumStoredBytes();
		byteArray.resize(numBytesStored);
		if (!ringBuffer.PeakBytes((uint8_t*)byteArray.data(), numBytesStored))
			break;

		std::string logMessage;
		for (int i = 0; i < numBytesStored; i++)
		{
			logMessage += byteArray[i];
			if (byteArray[i] == '\n')
			{
				this->AddLogMessage(logMessage);
				ringBuffer.DeleteBytes(i + 1);
				break;
			}
		}
	}

	client->exited = true;
}