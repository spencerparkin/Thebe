#include "ComputerClient.h"

ComputerClient::ComputerClient()
{
}

/*virtual*/ ComputerClient::~ComputerClient()
{
}

/*virtual*/ bool ComputerClient::HandleResponse(const ParseParty::JsonValue* jsonResponse)
{
	if (!ChineseCheckersClient::HandleResponse(jsonResponse))
		return false;

	// TODO: Take our turn when necessary.

	return true;
}