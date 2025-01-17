#pragma once

#include "Thebe/Reference.h"
#include "Thebe/Network/Address.h"
#include "Thebe/Network/JsonSocketReceiver.h"
#include "Thebe/Network/JsonSocketSender.h"

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API JsonServer
	{
		friend class ConnectedClient;

	public:
		JsonServer();
		virtual ~JsonServer();

		virtual bool Setup();
		virtual void Shutdown();
		virtual void Serve();

		SOCKET GetSocket();

		void SetAddress(const NetworkAddress& address);

	protected:
		class ConnectedClient;

		virtual void OnClientConnected(ConnectedClient* client);
		virtual void OnClientDisconnected(ConnectedClient* client);

		struct ClientMessage
		{
			const ParseParty::JsonValue* jsonValue;
			Reference<ConnectedClient> client;
		};

		virtual void ProcessClientMessage(ClientMessage* message);

		ThreadSafeQueue<ClientMessage> clientMessageQueue;

		class THEBE_API ClientManagerThread : public Thread
		{
		public:
			ClientManagerThread(JsonServer* server);
			virtual ~ClientManagerThread();

			virtual void Run() override;

			void SendJsonToAllClients(const ParseParty::JsonValue* jsonValue);

		private:
			JsonServer* server;
			std::list<Reference<ConnectedClient>> connectedClientList;
			std::mutex connectedClientListMutex;
		};

		class THEBE_API ConnectedClient : public ReferenceCounted
		{
		public:
			ConnectedClient(SOCKET connectedSocket, JsonServer* server);
			virtual ~ConnectedClient();

			bool Setup();
			void Shutdown();

			void SendJson(const ParseParty::JsonValue* jsonValue);

			bool StillConnected();

			uintptr_t GetUserData();
			void SetUserData(uintptr_t userData);

		private:
			void ReceiveJson(const ParseParty::JsonValue* jsonValue);

			uintptr_t userData;
			SOCKET connectedSocket;
			JsonSocketReceiver* receiver;
			JsonSocketSender* sender;
			JsonServer* server;
		};

		ClientManagerThread* clientManager;
		SOCKET socket;
		NetworkAddress address;
	};
}