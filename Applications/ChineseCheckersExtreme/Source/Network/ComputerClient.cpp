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

	return true;
}

/*virtual*/ void ComputerClient::Update(double deltaTimeSeconds)
{
	ChineseCheckersClient::Update(deltaTimeSeconds);

	if (this->whoseTurnZoneID == this->GetSourceZoneID())
	{
		// TODO: Tell our thread to start formulating a move sequence.
	}
}