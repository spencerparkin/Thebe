#include "Thebe/NetLog.h"

using namespace Thebe;

//------------------------------------ NetLogSink ------------------------------------

NetLogSink::NetLogSink()
{
	this->client = new NetworkClient();
}

/*virtual*/ NetLogSink::~NetLogSink()
{
	if (this->client)
	{
		this->client->Shutdown();
		delete this->client;
	}
}

/*virtual*/ bool NetLogSink::Setup()
{
	this->client->SetSocketFactory([](SOCKET socket) -> NetworkSocket* { return new NetworkSocket(socket, THEBE_NETWORK_SOCKET_FLAG_NEEDS_WRITING); });
	if (!this->client->Setup())
		return false;

	return true;
}

/*virtual*/ void NetLogSink::Print(const std::string& msg)
{
	if (this->client)
		this->client->GetSocket()->SendData((const uint8_t*)msg.c_str(), msg.length() + 1);
}

void NetLogSink::SetConnectAddress(const NetworkAddress& connectAddress)
{
	if (this->client)
		this->client->SetAddress(connectAddress);
}

//------------------------------------ NetLogCollector ------------------------------------

NetLogCollector::NetLogCollector()
{
}

/*virtual*/ NetLogCollector::~NetLogCollector()
{
}

/*virtual*/ bool NetLogCollector::Setup()
{
	this->SetSocketFactory([=](SOCKET socket) -> NetworkSocket* { return new Socket(socket, this); });

	if (!NetworkServer::Setup())
		return false;

	return true;
}

/*virtual*/ void NetLogCollector::Shutdown()
{
	NetworkServer::Shutdown();
}

void NetLogCollector::AddLogMessage(const std::string& msg)
{
	std::lock_guard<std::mutex> lock(this->logMessageListMutex);
	this->logMessageList.push_back(msg);
}

bool NetLogCollector::GetLogMessage(std::string& msg)
{
	if (this->logMessageList.size() > 0)
	{
		std::lock_guard<std::mutex> lock(this->logMessageListMutex);
		if (this->logMessageList.size() > 0)
		{
			msg = *this->logMessageList.begin();
			this->logMessageList.pop_front();
			return true;
		}
	}

	return false;
}

//------------------------------------ NetLogCollector::Socket ------------------------------------
		
NetLogCollector::Socket::Socket(SOCKET socket, NetLogCollector* collector) : NetworkSocket(socket, THEBE_NETWORK_SOCKET_FLAG_NEEDS_READING)
{
	this->collector = collector;
}

/*virtual*/ NetLogCollector::Socket::~Socket()
{
}

/*virtual*/ bool NetLogCollector::Socket::ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numBytesProcessed)
{
	numBytesProcessed = 0;

	bool nullTerminal = false;
	for (uint32_t i = 0; i < bufferSize && !nullTerminal; i++)
		if (buffer[i] == '\0')
			nullTerminal = true;

	if (nullTerminal)
	{
		std::string msg((const char*)buffer);
		this->collector->AddLogMessage(msg);
		numBytesProcessed = msg.length() + 1;
	}

	return true;
}