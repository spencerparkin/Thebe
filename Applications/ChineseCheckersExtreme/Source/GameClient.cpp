#include "GameClient.h"
#include "Factory.h"
#include "ChineseCheckers/MoveSequence.h"

//------------------------------------ ChineseCheckersGameClient ------------------------------------

ChineseCheckersGameClient::ChineseCheckersGameClient()
{
	this->color = ChineseCheckers::Marble::Color::NONE;
	this->whoseTurn = ChineseCheckers::Marble::Color::NONE;
}

/*virtual*/ ChineseCheckersGameClient::~ChineseCheckersGameClient()
{
}

ChineseCheckers::Graph* ChineseCheckersGameClient::GetGraph()
{
	return this->graph.get();
}

ChineseCheckers::Marble::Color ChineseCheckersGameClient::GetColor() const
{
	return this->color;
}

/*virtual*/ bool ChineseCheckersGameClient::Setup()
{
	if (!JsonClient::Setup())
		return false;

	this->factory.reset(new Factory());

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
	else if (response == "make_move")
	{
		ChineseCheckers::MoveSequence moveSequence;
		if (!moveSequence.FromJson(responseValue->GetValue("move_sequence")))
			return;

		ChineseCheckers::Graph::Move move;
		if (!moveSequence.ToMove(move, this->graph.get()))
			return;

		this->graph->MoveMarbleUnconditionally(move);

		auto whoseTurnValue = dynamic_cast<const JsonInt*>(responseValue->GetValue("whose_turn"));
		if (!whoseTurnValue)
			return;

		this->whoseTurn = (ChineseCheckers::Marble::Color)whoseTurnValue->GetValue();
	}
}

/*virtual*/ void ChineseCheckersGameClient::HandleConnectionStatus(ConnectionStatus status, int i, bool* abort)
{
}

void ChineseCheckersGameClient::MakeMove(const ChineseCheckers::MoveSequence& moveSequence)
{
	using namespace ParseParty;

	std::unique_ptr<JsonObject> requestValue(new JsonObject());
	requestValue->SetValue("request", new JsonString("make_move"));

	std::unique_ptr<JsonValue> moveSequenceValue;
	if (moveSequence.ToJson(moveSequenceValue))
	{
		requestValue->SetValue("move_sequence", moveSequenceValue.release());
		this->SendJson(requestValue.get());
	}
}