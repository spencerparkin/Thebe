#include "Thebe/Network/Socket.h"
#include "Thebe/Utilities/RingBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

//--------------------------------- NetworkSocket ---------------------------------

NetworkSocket::NetworkSocket(SOCKET socket)
{
	this->socket = socket;
	this->ringBufferSize = 2048;
	this->recvBufferSize = 128;
	this->periodicWakeupTimeSeconds = 0;
}

/*virtual*/ NetworkSocket::~NetworkSocket()
{
}

void NetworkSocket::SetPeriodicWakeup(long periodicWakeupTimeSeconds)
{
	this->periodicWakeupTimeSeconds = periodicWakeupTimeSeconds;
}

/*virtual*/ bool NetworkSocket::Join()
{
	if (this->socket != INVALID_SOCKET)
	{
		::closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	return Thread::Join();
}

/*virtual*/ void NetworkSocket::OnWakeup()
{
}

/*virtual*/ void NetworkSocket::Run()
{
	RingBuffer ringBuffer(this->ringBufferSize);
	std::unique_ptr<char> recvBuffer(new char[this->recvBufferSize]);
	std::vector<char> byteArray;

	while (true)
	{
		int result = 0;

		if (this->periodicWakeupTimeSeconds > 0)
		{
			timeval tval{};
			tval.tv_sec = this->periodicWakeupTimeSeconds;
			tval.tv_usec = 0;

			fd_set readSet, errorSet;
			FD_ZERO(&readSet);
			FD_ZERO(&errorSet);
			FD_SET(this->socket, &readSet);
			FD_SET(this->socket, &errorSet);

			result = ::select(0, &readSet, nullptr, &errorSet, &tval);
			if (result == SOCKET_ERROR)
			{
				THEBE_LOG("Select failed on socket.  Error: %d", WSAGetLastError());
				break;
			}

			if (FD_ISSET(this->socket, &errorSet))
			{
				THEBE_LOG("Select says our socket is in an error state.");
				break;
			}

			if (!FD_ISSET(this->socket, &readSet))
			{
				THEBE_LOG("Select says there's nothing to read.  Waiting again...");
				this->OnWakeup();
				continue;
			}
		}

		// Block here until we receive some data.
		WSABUF buffer;
		buffer.buf = recvBuffer.get();
		buffer.len = this->recvBufferSize;
		DWORD numBytesReceived = 0;
		DWORD flags = MSG_PUSH_IMMEDIATE;
		result = WSARecv(this->socket, (LPWSABUF)&buffer, 1, &numBytesReceived, &flags, nullptr, nullptr);
		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error == WSAECONNABORTED || error == WSAECONNRESET)
			{
				THEBE_LOG("Socket thread signaled to close.");
				break;
			}

			THEBE_LOG("Failed to receive data on socket.  Error: %d", error);
			break;
		}

		if (!ringBuffer.WriteBytes((uint8_t*)recvBuffer.get(), numBytesReceived))
			break;

		uint32_t numBytesStored = ringBuffer.GetNumStoredBytes();
		byteArray.resize(numBytesStored);
		if (!ringBuffer.PeakBytes((uint8_t*)byteArray.data(), numBytesStored))
			break;

		// This is where the derived class can process the data.  For server-side,
		// this may mean responding to a request; client-side, storing the response.
		uint32_t numBytesProcessed = 0;
		if (!this->ReceiveData((uint8_t*)byteArray.data(), numBytesStored, numBytesProcessed))
			break;

		THEBE_ASSERT(numBytesProcessed <= numBytesStored);
		if (!ringBuffer.DeleteBytes(THEBE_MIN(numBytesProcessed, numBytesStored)))
			break;
	}
}

/*virtual*/ bool NetworkSocket::ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numbBytesProcessed)
{
	// By default, just pretend to receive all the data.
	numbBytesProcessed = bufferSize;
	return true;
}

bool NetworkSocket::SendData(const uint8_t* buffer, uint32_t bufferSize)
{
	// Note that instead of looping here until it's all sent,
	// a better way might be to add all the data to a stream
	// that we are constantly trying to flush down the socket
	// whenever we have some spare time.

	uint32_t totalBytesSent = 0;
	while (totalBytesSent < bufferSize)
	{
		uint32_t numBytesRemaining = bufferSize - totalBytesSent;
		uint32_t numBytesSent = ::send(this->socket, (const char*)&buffer[totalBytesSent], numBytesRemaining, 0);
		if (numBytesSent == SOCKET_ERROR)
		{
			THEBE_LOG("Failed to send data through socket.  Error: %d", WSAGetLastError());
			return false;
		}

		totalBytesSent += numBytesSent;
	}

	return true;
}

//--------------------------------- JsonNetworkSocket ---------------------------------

JsonNetworkSocket::JsonNetworkSocket(SOCKET socket) : NetworkSocket(socket)
{
}

/*virtual*/ JsonNetworkSocket::~JsonNetworkSocket()
{
}

/*virtual*/ bool JsonNetworkSocket::ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numBytesProcessed)
{
	using namespace ParseParty;

	numBytesProcessed = 0;

	bool nullTerminated = false;
	for (uint32_t i = 0; i < bufferSize && !nullTerminated; i++)
		if (buffer[i] == '\0')
			nullTerminated = true;

	if (!nullTerminated)
		return true;

	std::string jsonString((const char*)buffer);
	std::string parseError;
	std::unique_ptr<JsonValue> jsonRootValue(JsonValue::ParseJson(jsonString, parseError));
	if (!jsonRootValue.get())
		return false;

	numBytesProcessed = jsonString.length() + 1;

	if (!this->ReceiveJson(jsonRootValue))
		return false;

	return true;
}

/*virtual*/ bool JsonNetworkSocket::ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue)
{
	return true;
}

bool JsonNetworkSocket::SendJson(const ParseParty::JsonValue* jsonRootValue)
{
	std::string jsonString;
	if (!jsonRootValue->PrintJson(jsonString))
		return false;

	if (!this->SendData((const uint8_t*)jsonString.c_str(), jsonString.length() + 1))
		return false;

	return true;
}