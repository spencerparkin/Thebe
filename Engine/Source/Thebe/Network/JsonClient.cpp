#include "Thebe/Network/JsonClient.h"

using namespace Thebe;

JsonClient::JsonClient()
{
	this->connectedSocket = INVALID_SOCKET;
	this->receiver = nullptr;
	this->sender = nullptr;
	this->maxConnectionAttempts = 10;
	this->retryWaitTimeMilliseconds = 200;
	this->needsSending = true;
	this->needsReceiving = true;
}

/*virtual*/ JsonClient::~JsonClient()
{
}

void JsonClient::SetNeeds(bool needsSending, bool needsReceiving)
{
	this->needsSending = needsSending;
	this->needsReceiving = needsReceiving;
}

/*virtual*/ bool JsonClient::Setup()
{
	if (this->sender || this->receiver)
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

	this->connectedSocket = ::socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
	if (this->connectedSocket == INVALID_SOCKET)
		return false;

	bool connected = false;
	for (int i = 0; i < this->maxConnectionAttempts; i++)
	{
		int result = ::connect(this->connectedSocket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
		if (result != SOCKET_ERROR)
		{
			connected = true;
			break;
		}

		::Sleep(this->retryWaitTimeMilliseconds);
	}

	freeaddrinfo(addressInfo);

	if (!connected)
		return false;

	if (this->needsReceiving)
	{
		this->receiver = new JsonSocketReceiver(this->connectedSocket);
		this->receiver->SetRecvFunc([=](std::unique_ptr<ParseParty::JsonValue>& jsonValue) { this->jsonMessageQueue.Add(jsonValue.release()); });
		if (!this->receiver->Split())
			return false;
	}

	if (this->needsSending)
	{
		this->sender = new JsonSocketSender(this->connectedSocket);
		if (!this->sender->Split())
			return false;
	}

	return true;
}

/*virtual*/ void JsonClient::Shutdown()
{
	if (this->connectedSocket != INVALID_SOCKET)
	{
		::closesocket(this->connectedSocket);
		this->connectedSocket = INVALID_SOCKET;
	}

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

/*virtual*/ void JsonClient::Update()
{
	const ParseParty::JsonValue* jsonValue = nullptr;
	if (this->jsonMessageQueue.Remove(jsonValue))
	{
		this->ProcessServerMessage(jsonValue);
		delete jsonValue;
	}
}

void JsonClient::SetAddress(const NetworkAddress& address)
{
	this->address = address;
}

void JsonClient::SendJson(const ParseParty::JsonValue* jsonValue)
{
	if (this->sender)
		this->sender->SendJson(jsonValue);
}

/*virtual*/ void JsonClient::ProcessServerMessage(const ParseParty::JsonValue* jsonValue)
{
}