#include "HumanClient.h"

HumanClient::HumanClient()
{
}

/*virtual*/ HumanClient::~HumanClient()
{
}

/*virtual*/ bool HumanClient::HandleResponse(const ParseParty::JsonValue* jsonResponse)
{
	using namespace ParseParty;

	if (!ChineseCheckersClient::HandleResponse(jsonResponse))
		return false;

	std::string response = ((const JsonString*)((const JsonObject*)jsonResponse)->GetValue("response"))->GetValue();
	if (response == "get_game_state")
	{
		// TODO: Load all mesh and mesh instances into the scene.
	}

	return true;
}