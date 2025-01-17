#pragma once

#include "Thebe/Network/Address.h"
#include "Thebe/Network/JsonSocketReceiver.h"
#include "Thebe/Network/JsonSocketSender.h"
#include "JsonValue.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API JsonClient
	{
	public:
		JsonClient();
		virtual ~JsonClient();

		virtual bool Setup();
		virtual void Shutdown();
		virtual void Update();

		void SetAddress(const NetworkAddress& address);

		void SendJson(const ParseParty::JsonValue* jsonValue);

	protected:
		virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue);

		SOCKET connectedSocket;
		NetworkAddress address;
		JsonSocketReceiver* receiver;
		JsonSocketSender* sender;
		ThreadSafeQueue<const ParseParty::JsonValue*> jsonMessageQueue;
		int maxConnectionAttempts;
		int retryWaitTimeMilliseconds;
	};
}