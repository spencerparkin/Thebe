#pragma once

#include "Thebe/Common.h"
#include "Thebe/Network/Address.h"
#include "Thebe/Utilities/Thread.h"
#include "JsonValue.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <functional>

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API NetworkSocket : public Thread
	{
	public:
		NetworkSocket(SOCKET socket);
		virtual ~NetworkSocket();

		virtual bool ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numBytesProcessed);

		bool SendData(const uint8_t* buffer, uint32_t bufferSize);

		virtual bool Join() override;
		virtual void Run() override;

	protected:
		int ringBufferSize;
		int recvBufferSize;
		SOCKET socket;
	};

	typedef std::function<NetworkSocket*(SOCKET socket)> SocketFactoryFunction;

	/**
	 *
	 */
	class THEBE_API JsonNetworkSocket : public NetworkSocket
	{
	public:
		JsonNetworkSocket(SOCKET socket);
		virtual ~JsonNetworkSocket();

		virtual bool ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numBytesProcessed) override;

		virtual bool ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue);

		bool SendJson(const ParseParty::JsonValue* jsonRootValue);
	};
}