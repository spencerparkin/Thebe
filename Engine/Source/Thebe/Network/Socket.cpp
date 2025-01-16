#include "Thebe/Network/Socket.h"
#include "Thebe/Utilities/RingBuffer.h"
#include "Thebe/Log.h"

using namespace Thebe;

//--------------------------------- NetworkSocketReader ---------------------------------

NetworkSocketReader::NetworkSocketReader(NetworkSocket* networkSocket)
{
	this->networkSocket = networkSocket;
	this->ringBufferSize = 64 * 1024;
	this->recvBufferSize = 4 * 1024;
}

/*virtual*/ NetworkSocketReader::~NetworkSocketReader()
{
}

/*virtual*/ void NetworkSocketReader::Run()
{
	RingBuffer ringBuffer(this->ringBufferSize);
	std::unique_ptr<char> recvBuffer(new char[this->recvBufferSize]);
	std::vector<char> byteArray;

	while (true)
	{
		// Block here until we have something to read from the socket.
		uint32_t numBytesReceived = ::recv(this->networkSocket->GetSocket(), recvBuffer.get(), recvBufferSize, 0);
		if (numBytesReceived == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error == WSAECONNABORTED || error == WSAECONNRESET)
				break;

			THEBE_LOG("Failed to receive data on socket.  Error: %d", error);
			break;
		}

		if (!ringBuffer.WriteBytes((uint8_t*)recvBuffer.get(), numBytesReceived))
			break;

		uint32_t numBytesStored = ringBuffer.GetNumStoredBytes();
		byteArray.resize(numBytesStored);
		if (!ringBuffer.PeakBytes((uint8_t*)byteArray.data(), numBytesStored))
			break;

		uint32_t numBytesProcessed = 0;
		if (!this->networkSocket->ReceiveData((uint8_t*)byteArray.data(), numBytesStored, numBytesProcessed))
			break;

		THEBE_ASSERT(numBytesProcessed <= numBytesStored);
		if (!ringBuffer.DeleteBytes(THEBE_MIN(numBytesProcessed, numBytesStored)))
			break;
	}
}

//--------------------------------- NetworkSocketWriter ---------------------------------

NetworkSocketWriter::NetworkSocketWriter(NetworkSocket* networkSocket) : writeListSemaphore(0)
{
	this->networkSocket = networkSocket;
}

/*virtual*/ NetworkSocketWriter::~NetworkSocketWriter()
{
	for (Write& write : this->writeList)
		delete[] write.buffer;
}

bool NetworkSocketWriter::WriteData(const uint8_t* buffer, uint32_t bufferSize)
{
	Write write{};

	if (bufferSize > 0)
	{
		write.buffer = new uint8_t[bufferSize];
		::memcpy(write.buffer, buffer, bufferSize);
		write.bufferSize = bufferSize;
	}

	// Enter mutex lock scope.
	{
		std::lock_guard lock(this->writeListMutex);
		this->writeList.push_back(write);
	}

	this->writeListSemaphore.release();
	return true;
}

/*virtual*/ void NetworkSocketWriter::Run()
{
	while (true)
	{
		// Block here until we have something to write to the socket.
		this->writeListSemaphore.acquire();

		Write write{};
		{
			std::lock_guard lock(this->writeListMutex);
			if (this->writeList.size() > 0)
			{
				write = *this->writeList.begin();
				this->writeList.pop_front();
			}
		}

		if (!write.buffer)
			break;

		uint32_t totalBytesSent = 0;
		while (totalBytesSent < write.bufferSize)
		{
			uint32_t numBytesRemaining = write.bufferSize - totalBytesSent;
			uint32_t numBytesSent = ::send(this->networkSocket->GetSocket(), (const char*)&write.buffer[totalBytesSent], numBytesRemaining, 0);
			if (numBytesSent == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				if (error == WSAECONNABORTED || error == WSAECONNRESET)
					return;

				THEBE_LOG("Failed to send data through socket.  Error: %d", WSAGetLastError());
				return;
			}

			totalBytesSent += numBytesSent;
		}

		delete[] write.buffer;

		// Please flush the socket.  Please!  Note that SIO_FLUSH is not supported on windows.  Also, TCP_NODELAY does nothing to help.
		// I don't know why this sleep seems to matter, but it does.  I really wish I understand what the problem was really all about.
		::Sleep(100);
	}
}

//--------------------------------- NetworkSocket ---------------------------------

NetworkSocket::NetworkSocket(SOCKET socket, uint32_t flags)
{
	this->socket = socket;
	this->flags = flags;
	this->reader = nullptr;
	this->writer = nullptr;
}

/*virtual*/ NetworkSocket::~NetworkSocket()
{
	THEBE_ASSERT(this->reader == nullptr);
	THEBE_ASSERT(this->writer == nullptr);
}

SOCKET NetworkSocket::GetSocket()
{
	return this->socket;
}

bool NetworkSocket::Setup()
{
	if (this->reader || this->writer)
		return false;

	if ((this->flags & THEBE_NETWORK_SOCKET_FLAG_NEEDS_READING) != 0)
	{
		this->reader = new NetworkSocketReader(this);
		if (!this->reader->Split())
			return false;
	}

	if ((this->flags & THEBE_NETWORK_SOCKET_FLAG_NEEDS_WRITING) != 0)
	{
		this->writer = new NetworkSocketWriter(this);
		if (!this->writer->Split())
			return false;
	}

	return true;
}

void NetworkSocket::Shutdown()
{
	if (this->socket != INVALID_SOCKET)
	{
		::closesocket(this->socket);
		this->socket = INVALID_SOCKET;
	}

	if (this->reader)
	{
		this->reader->Join();
		delete this->reader;
		this->reader = nullptr;
	}

	if (this->writer)
	{
		this->writer->WriteData(nullptr, 0);
		this->writer->Join();
		delete this->writer;
		this->writer = nullptr;
	}
}

bool NetworkSocket::IsReaderRunning()
{
	return this->reader && this->reader->IsRunning();
}

bool NetworkSocket::IsWriterRunning()
{
	return this->writer && this->writer->IsRunning();
}

/*virtual*/ bool NetworkSocket::ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numbBytesProcessed)
{
	// By default, just pretend to receive all the data.
	numbBytesProcessed = bufferSize;
	return true;
}

bool NetworkSocket::SendData(const uint8_t* buffer, uint32_t bufferSize)
{
	if (!this->writer)
		return false;

	return this->writer->WriteData(buffer, bufferSize);
}

//--------------------------------- JsonNetworkSocket ---------------------------------

JsonNetworkSocket::JsonNetworkSocket(SOCKET socket, uint32_t flags) : NetworkSocket(socket, flags)
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