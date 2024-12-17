#include "Thebe/DatagramLog.h"
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

void NetworkAddress::SetPort(uint32_t port)
{
	this->port = port;
}

void NetworkAddress::GetSockAddr(sockaddr_in& addr) const
{
	::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = ::inet_addr(this->ipAddr.c_str());
	addr.sin_port = htons(this->port);
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
		int result = ::sendto(this->socket, msg.c_str(), msg.length(), 0, (const sockaddr*)&addr, sizeof(addr));
		result = 0;
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

bool DatagramLogCollector::GrabLogMessage(std::string& logMessage)
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
		{
			std::scoped_lock<std::mutex> lock(this->logMessageListMutex);
			this->logMessageList.push_back(logMessage);
		}
	}
}