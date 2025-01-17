#include "ComputerClient.h"

ComputerClient::ComputerClient()
{
}

/*virtual*/ ComputerClient::~ComputerClient()
{
}

/*virtual*/ void ComputerClient::Update()
{
	ChineseCheckersClient::Update();

	if (this->whoseTurnZoneID == this->GetSourceZoneID())
	{
		// TODO: Formulate and send a move sequence here to the server.
	}
}