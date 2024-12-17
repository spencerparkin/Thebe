#pragma once

#include "Thebe/Log.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <list>
#include <thread>

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API NetworkAddress
	{
	public:
		NetworkAddress();
		NetworkAddress(const NetworkAddress& address);
		virtual ~NetworkAddress();

		void operator=(const NetworkAddress& address);

		bool SetAddress(const std::string& ipAddrAndPort);
		std::string GetAddress() const;
		void SetIPAddress(const std::string& ipAddr);
		void SetPort(uint32_t port);
		void GetSockAddr(sockaddr_in& addr) const;

	private:
		std::string ipAddr;
		uint32_t port;
	};

	/**
	 * Send log messages over UDP to a collector.
	 */
	class THEBE_API DatagramLogSink : public LogSink
	{
	public:
		DatagramLogSink();
		virtual ~DatagramLogSink();

		virtual bool Setup() override;
		virtual void Print(const std::string& msg) override;

		void SetSendAddress(const NetworkAddress& sendAddress);

	private:
		NetworkAddress sendAddress;
		SOCKET socket;
	};

	/**
	 * This can be owned by a host application that consumes the log messages.
	 */
	class THEBE_API DatagramLogCollector
	{
	public:
		DatagramLogCollector();
		virtual ~DatagramLogCollector();

		bool Setup();
		void Shutdown();
		bool GrabLogMessage(std::string& logMessage);

		void SetReceptionAddress(const NetworkAddress& receptionAddress);
		const NetworkAddress& GetReceptionAddress() const;

	private:
		void ThreadRun();

		NetworkAddress receptionAddress;
		SOCKET socket;
		std::thread* thread;
		std::list<std::string> logMessageList;
		std::mutex logMessageListMutex;
	};
}