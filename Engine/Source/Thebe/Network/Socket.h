#pragma once

#include "Thebe/Common.h"
#include "Thebe/Network/Address.h"
#include "Thebe/Utilities/Thread.h"
#include "Thebe/Reference.h"
#include "JsonValue.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <functional>
#include <semaphore>
#include <mutex>

#define THEBE_NETWORK_SOCKET_FLAG_NEEDS_READING			0x00000001
#define THEBE_NETWORK_SOCKET_FLAG_NEEDS_WRITING			0x00000002

namespace Thebe
{
	class NetworkSocket;

	/**
	 * This is a dedicated thread for reading from a network end-point.
	 *
	 * Being a thread, we can block while trying to read from the socket
	 * without halting the rest of the program.  Ideally we'd be signaled
	 * (woken up) near the moment when data is available to read on the
	 * socket.
	 *
	 * The socket being closed from another thread will signal us to exit.
	 */
	class THEBE_API NetworkSocketReader : public Thread
	{
	public:
		NetworkSocketReader(NetworkSocket* networkSocket);
		virtual ~NetworkSocketReader();

		virtual void Run() override;

	protected:
		NetworkSocket* networkSocket;
		int ringBufferSize;
		int recvBufferSize;
	};

	/**
	 * This is a dedicated thread for writing to a network end-point.
	 *
	 * One of the key ideas here is that perhaps it is critical that
	 * a thread that writes to a socket is able to go to sleep (by
	 * waiting on a semaphore, or otherwise) so that the data written
	 * will actually get flushed to the network.  I'm not sure exactly
	 * why this is necessary, but my instincts tell me that it is.
	 * 
	 * In any case, it is possible for a write operation to block, and
	 * if that ever happened, we wouldn't want that to impede the main
	 * program flow.
	 *
	 * Attempting to write a zero-byte buffer will signal us to exit.
	 */
	class THEBE_API NetworkSocketWriter : public Thread
	{
	public:
		NetworkSocketWriter(NetworkSocket* networkSocket);
		virtual ~NetworkSocketWriter();

		bool WriteData(const uint8_t* buffer, uint32_t bufferSize);

		virtual void Run() override;

	protected:
		struct Write
		{
			uint8_t* buffer;
			uint32_t bufferSize;
		};

		NetworkSocket* networkSocket;
		std::counting_semaphore<1024> writeListSemaphore;
		std::mutex writeListMutex;
		std::list<Write> writeList;
	};

	/**
	 *
	 */
	class THEBE_API NetworkSocket : public ReferenceCounted
	{
	public:
		NetworkSocket(SOCKET socket, uint32_t flags);
		virtual ~NetworkSocket();

		bool Setup();
		void Shutdown();

		virtual bool ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numBytesProcessed);

		bool SendData(const uint8_t* buffer, uint32_t bufferSize);

		SOCKET GetSocket();

		bool IsReaderRunning();
		bool IsWriterRunning();

	protected:
		NetworkSocketReader* reader;
		NetworkSocketWriter* writer;
		uint32_t flags;
		SOCKET socket;
	};

	typedef std::function<NetworkSocket*(SOCKET socket)> SocketFactoryFunction;

	/**
	 *
	 */
	class THEBE_API JsonNetworkSocket : public NetworkSocket
	{
	public:
		JsonNetworkSocket(SOCKET socket, uint32_t flags);
		virtual ~JsonNetworkSocket();

		virtual bool ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numBytesProcessed) override;

		virtual bool ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue);

		bool SendJson(const ParseParty::JsonValue* jsonRootValue);
	};
}