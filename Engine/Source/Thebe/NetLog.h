#pragma once

#include "Thebe/Log.h"
#include "Thebe/Network/JsonClient.h"
#include "Thebe/Network/JsonServer.h"
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
		JsonClient* client;
	};

	/**
	 *
	 */
	class THEBE_API NetLogCollector : public JsonServer
	{
	public:
		NetLogCollector();
		virtual ~NetLogCollector();

		virtual bool Setup() override;

		bool GetLogMessage(std::string& msg);

	private:
		virtual void ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply) override;

		std::list<std::string> logMsgList;
	};
}