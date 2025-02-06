#include "ComputerClient.h"

ComputerClient::ComputerClient()
{
}

/*virtual*/ ComputerClient::~ComputerClient()
{
}

/*virtual*/ void ComputerClient::ProcessServerMessage(const ParseParty::JsonValue* jsonValue)
{
	ChineseCheckersGameClient::ProcessServerMessage(jsonValue);

	//...
}