#include "GameClient.h"
#include "Factory.h"

ChineseCheckersGameClient::ChineseCheckersGameClient()
{
	this->color = ChineseCheckers::Marble::Color::NONE;
	this->whoseTurn = ChineseCheckers::Marble::Color::NONE;
}

/*virtual*/ ChineseCheckersGameClient::~ChineseCheckersGameClient()
{
}

/*virtual*/ bool ChineseCheckersGameClient::Setup()
{
	if (!this->factory.get())
		return false;

	if (!JsonClient::Setup())
		return false;

	using namespace ParseParty;

	std::unique_ptr<JsonObject> requestValue(new JsonObject());
	requestValue->SetValue("request", new JsonString("get_graph"));
	this->SendJson(requestValue.get());

	requestValue.reset(new JsonObject());
	requestValue->SetValue("request", new JsonString("get_color"));
	this->SendJson(requestValue.get());

	requestValue.reset(new JsonObject());
	requestValue->SetValue("request", new JsonString("get_whose_turn"));
	this->SendJson(requestValue.get());

	return true;
}

/*virtual*/ void ChineseCheckersGameClient::Shutdown()
{
	JsonClient::Shutdown();
}

/*virtual*/ void ChineseCheckersGameClient::Update(double deltaTimeSeconds)
{
	JsonClient::Update(deltaTimeSeconds);
}

/*virtual*/ void ChineseCheckersGameClient::ProcessServerMessage(const ParseParty::JsonValue* jsonValue)
{
	using namespace ParseParty;

	auto responseValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!responseValue)
		return;

	auto responseTypeValue = dynamic_cast<const JsonString*>(responseValue->GetValue("response"));
	if (!responseTypeValue)
		return;

	std::string response = responseTypeValue->GetValue();

	if (response == "get_graph")
	{
		auto graphValue = responseValue->GetValue("graph");
		if (!graphValue)
			return;

		if (!this->graph.get())
			this->graph.reset(this->factory->CreateGraph());

		this->graph->FromJson(graphValue, this->factory.get());
	}
	else if (response == "get_color")
	{
		auto colorValue = dynamic_cast<const JsonInt*>(responseValue->GetValue("color"));
		if (!colorValue)
			return;

		this->color = (ChineseCheckers::Marble::Color)colorValue->GetValue();
	}
	else if (response == "get_whose_turn")
	{
		auto whoseTurnValue = dynamic_cast<const JsonInt*>(responseValue->GetValue("whose_turn"));
		if (!whoseTurnValue)
			return;

		this->whoseTurn = (ChineseCheckers::Marble::Color)whoseTurnValue->GetValue();
	}
}

/*virtual*/ void ChineseCheckersGameClient::HandleConnectionStatus(ConnectionStatus status, int i, bool* abort)
{
}