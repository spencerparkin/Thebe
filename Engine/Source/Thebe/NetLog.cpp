#include "Thebe/NetLog.h"
#include "JsonValue.h"

using namespace Thebe;

//------------------------------------ NetLogSink ------------------------------------

NetLogSink::NetLogSink()
{
	this->client = new JsonClient();
}

/*virtual*/ NetLogSink::~NetLogSink()
{
	if (this->client)
	{
		this->client->Shutdown();
		delete this->client;
	}
}

/*virtual*/ bool NetLogSink::Setup()
{
	this->client->SetNeeds(true, false);

	return this->client->Setup();
}

/*virtual*/ void NetLogSink::Print(const std::string& msg)
{
	using namespace ParseParty;

	if (this->client)
	{
		std::unique_ptr<JsonString> msgValue(new JsonString());
		msgValue->SetValue(msg);
		this->client->SendJson(msgValue.get());
	}
}

void NetLogSink::SetConnectAddress(const NetworkAddress& connectAddress)
{
	if (this->client)
		this->client->SetAddress(connectAddress);
}

//------------------------------------ NetLogCollector ------------------------------------

NetLogCollector::NetLogCollector()
{
}

/*virtual*/ NetLogCollector::~NetLogCollector()
{
}

/*virtual*/ bool NetLogCollector::Setup()
{
	this->SetNeeds(false, true);

	return JsonServer::Setup();
}

/*virtual*/ void NetLogCollector::ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply)
{
	using namespace ParseParty;

	auto msgValue = dynamic_cast<const JsonString*>(message->jsonValue);
	if (msgValue)
	{
		const std::string& msg = msgValue->GetValue();
		this->logMsgList.push_back(msg);
	}
}

bool NetLogCollector::GetLogMessage(std::string& msg)
{
	if (this->logMsgList.size() == 0)
		return false;

	msg = *this->logMsgList.begin();
	this->logMsgList.pop_front();
	return true;
}