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
		const std::string& GetIPAddress() const;
		void SetPort(uint32_t port);
		uint32_t GetPort() const;
		void GetSockAddr(sockaddr_in& addr) const;

	private:
		std::string ipAddr;
		uint32_t port;
	};

	/**
	 * Define an interface to log collection on a host.
	 */
	class THEBE_API NetLogCollector
	{
	public:
		NetLogCollector();
		virtual ~NetLogCollector();

		virtual bool Setup() = 0;
		virtual void Shutdown() = 0;
		
		bool RemoveLogMessage(std::string& logMessage);
		void AddLogMessage(const std::string& logMessage);

	protected:
		std::list<std::string> logMessageList;
		std::mutex logMessageListMutex;
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
	class THEBE_API DatagramLogCollector : public NetLogCollector
	{
	public:
		DatagramLogCollector();
		virtual ~DatagramLogCollector();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void SetReceptionAddress(const NetworkAddress& receptionAddress);
		const NetworkAddress& GetReceptionAddress() const;

	private:
		void ThreadRun();

		NetworkAddress receptionAddress;
		SOCKET socket;
		std::thread* thread;
	};

	/**
	 * Send log messages over TCP to a collector.
	 */
	class THEBE_API NetClientLogSink : public LogSink
	{
	public:
		NetClientLogSink();
		virtual ~NetClientLogSink();

		virtual bool Setup() override;
		virtual void Print(const std::string& msg) override;

		void SetConnectAddress(const NetworkAddress& connectAddress);

	private:
		NetworkAddress connectAddress;
		SOCKET socket;
	};

	/**
	 * This can also be owned by a host application that consumes the log messages.
	 */
	class THEBE_API NetServerLogCollector : public NetLogCollector
	{
	public:
		NetServerLogCollector();
		virtual ~NetServerLogCollector();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void SetListeningAddress(const NetworkAddress& listenAddress);

	private:
		NetworkAddress listenAddress;

		struct Client;

		void ListenThreadRun();
		void ClientThreadRun(Client* client);
		void RemoveExitedClients();

		std::thread* listenThread;
		SOCKET listenSocket;

		struct Client
		{
			std::thread* thread;
			SOCKET socket;
			bool exited;
		};

		std::list<Client*> clientList;
	};
}