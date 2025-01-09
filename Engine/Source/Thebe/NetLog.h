#pragma once

#include "Thebe/Log.h"
#include "Thebe/Network/Client.h"
#include "Thebe/Network/Server.h"
#include <mutex>

namespace Thebe
{
	/**
	 *
	 */
	class THEBE_API NetLogSink : public LogSink
	{
	public:
		NetLogSink();
		virtual ~NetLogSink();

		virtual bool Setup() override;
		virtual void Print(const std::string& msg) override;

		void SetConnectAddress(const NetworkAddress& connectAddress);

	private:
		NetworkClient* client;
	};

	/**
	 *
	 */
	class THEBE_API NetLogCollector : public NetworkServer
	{
	public:
		NetLogCollector();
		virtual ~NetLogCollector();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void AddLogMessage(const std::string& msg);
		bool GetLogMessage(std::string& msg);

	private:

		class Socket : public NetworkSocket
		{
		public:
			Socket(SOCKET socket, NetLogCollector* collector);
			virtual ~Socket();

			virtual bool ReceiveData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& numBytesProcessed) override;

		private:
			NetLogCollector* collector;
		};

		std::list<std::string> logMessageList;
		std::mutex logMessageListMutex;
	};
}