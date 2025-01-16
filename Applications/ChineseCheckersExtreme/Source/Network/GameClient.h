#pragma once

#include "Application.h"
#include "Thebe/Network/Client.h"
#include "Game.h"

/**
 *
 */
class ChineseCheckersClient : public Thebe::NetworkClient
{
public:
	ChineseCheckersClient();
	virtual ~ChineseCheckersClient();

	virtual bool Setup() override;
	virtual void Shutdown() override;

	virtual void Update(double deltaTimeSeconds);

	ChineseCheckersGame* GetGame();

	class Socket : public Thebe::JsonNetworkSocket
	{
	public:
		Socket(SOCKET socket, ChineseCheckersClient* client);
		virtual ~Socket();

		virtual bool ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonRootValue) override;

		ChineseCheckersClient* client;
		int sourceZoneID;
	};

	virtual bool HandleResponse(const ParseParty::JsonValue* jsonResponse);

	void AddResponse(std::unique_ptr<ParseParty::JsonValue>& jsonResponse);

	int GetSourceZoneID();

protected:
	Thebe::Reference<ChineseCheckersGame> game;
	int whoseTurnZoneID;

private:
	bool RemoveResponse(std::unique_ptr<const ParseParty::JsonValue>& jsonResponse);
	
	Thebe::ThreadSafeQueue<const ParseParty::JsonValue*> responseQueue;
	double timeToNextPingSeconds;
	double pingFrequencySecondsPerPing;
};