#include "HumanClient.h"

HumanClient::HumanClient()
{
}

/*virtual*/ HumanClient::~HumanClient()
{
}

/*virtual*/ bool HumanClient::HandleResponse(const ParseParty::JsonValue* jsonResponse)
{
	if (!ChineseCheckersClient::HandleResponse(jsonResponse))
		return false;

	// TODO: Update the scene accordingly.

	return true;
}