#include "GameServer.h"
#include "ChineseCheckers/Generators/TraditionalGenerator.h"
#include "ChineseCheckers/MoveSequence.h"
#include "Factory.h"

ChineseCheckersGameServer::ChineseCheckersGameServer()
{
	this->whoseTurn = ChineseCheckers::Marble::Color::NONE;
}

/*virtual*/ ChineseCheckersGameServer::~ChineseCheckersGameServer()
{
}

void ChineseCheckersGameServer::SetNumPlayers(int numPlayers)
{
	for (int i = 1; i <= numPlayers; i++)
		this->participantSet.insert((ChineseCheckers::Marble::Color)i);
}

/*virtual*/ bool ChineseCheckersGameServer::Setup()
{
	if (this->graph.get())
		return false;

	this->factory.reset(new Factory());

	this->graphGenerator.reset(new ChineseCheckers::TraditionalGenerator(this->factory.get()));
	this->graphGenerator->SetScale(10.0);

	if (!this->graphGenerator.get())
		return false;

	if (this->participantSet.size() == 0)
		return false;

	if (!JsonServer::Setup())
		return false;

	this->graph.reset(this->graphGenerator->Generate(this->participantSet));
	if (!graph)
		return false;

	for (ChineseCheckers::Marble::Color color : this->participantSet)
		this->freeColorStack.push_back(color);

	this->whoseTurn = this->freeColorStack[0];

	return true;
}

/*virtual*/ void ChineseCheckersGameServer::Shutdown()
{
	JsonServer::Shutdown();
}

/*virtual*/ void ChineseCheckersGameServer::Serve()
{
	JsonServer::Serve();
}

/*virtual*/ void ChineseCheckersGameServer::OnClientConnected(ConnectedClient* client)
{
	if (this->freeColorStack.size() == 0)
		client->SetUserData(ChineseCheckers::Marble::Color::NONE);
	else
	{
		ChineseCheckers::Marble::Color color = this->freeColorStack.back();
		this->freeColorStack.pop_back();
		client->SetUserData(color);
	}
}

/*virtual*/ void ChineseCheckersGameServer::OnClientDisconnected(ConnectedClient* client)
{
	auto color = (ChineseCheckers::Marble::Color)client->GetUserData();
	if (color != ChineseCheckers::Marble::Color::NONE)
		this->freeColorStack.push_back(color);
}

/*virtual*/ void ChineseCheckersGameServer::ProcessClientMessage(ClientMessage* message, std::unique_ptr<ParseParty::JsonValue>& jsonReply)
{
	using namespace ParseParty;

	auto requestValue = dynamic_cast<const JsonObject*>(message->jsonValue);
	if (!requestValue)
		return;

	auto requestTypeValue = dynamic_cast<const JsonString*>(requestValue->GetValue("request"));
	if (!requestTypeValue)
		return;

	std::string request = requestTypeValue->GetValue();

	if (request == "get_graph")
	{
		auto responseValue = new JsonObject();
		jsonReply.reset(responseValue);

		responseValue->SetValue("response", new JsonString("get_graph"));

		std::unique_ptr<JsonValue> graphValue;
		if (!this->graph->ToJson(graphValue))
			return;

		responseValue->SetValue("graph", graphValue.release());
	}
	else if (request == "get_color")
	{
		auto responseValue = new JsonObject();
		jsonReply.reset(responseValue);

		responseValue->SetValue("response", new JsonString("get_color"));
		responseValue->SetValue("color", new JsonInt(message->client->GetUserData()));
	}
	else if (request == "get_whose_turn")
	{
		auto responseValue = new JsonObject();
		jsonReply.reset(responseValue);

		responseValue->SetValue("response", new JsonString("get_whose_turn"));
		responseValue->SetValue("whose_turn", new JsonInt(this->whoseTurn));
	}
	else if (request == "make_move")
	{
		ChineseCheckers::MoveSequence moveSequence;
		if (!moveSequence.FromJson(requestValue->GetValue("move_sequence")))
			return;
		
		if (!this->graph->IsValidMoveSequence(moveSequence))
			return;

		ChineseCheckers::Node* node = this->graph->GetNodeArray()[moveSequence.nodeIndexArray[0]];
		if (node->GetOccupant()->GetColor() != this->whoseTurn)
			return;

		if (!this->graph->MoveMarbleConditionally(moveSequence))
			return;

		std::vector<ChineseCheckers::Marble::Color> colorArray;
		for (ChineseCheckers::Marble::Color color : this->participantSet)
			colorArray.push_back(color);

		for (int i = 0; i < (int)colorArray.size(); i++)
		{
			if (colorArray[i] == this->whoseTurn)
			{
				this->whoseTurn = colorArray[(i + 1) % int(colorArray.size())];
				break;
			}
		}

		std::unique_ptr<JsonObject> responseValue(new JsonObject());
		responseValue->SetValue("response", new JsonString("make_move"));

		std::unique_ptr<JsonValue> moveSequenceValue;
		if (!moveSequence.ToJson(moveSequenceValue))
			return;

		responseValue->SetValue("move_sequence", moveSequenceValue.release());
		responseValue->SetValue("whose_turn", new JsonInt(this->whoseTurn));

		this->clientManager->SendJsonToAllClients(responseValue.get());
	}
}