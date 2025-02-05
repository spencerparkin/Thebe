#include "HumanClient.h"

HumanClient::HumanClient()
{
}

/*virtual*/ HumanClient::~HumanClient()
{
}

/*virtual*/ void HumanClient::ProcessServerMessage(const ParseParty::JsonValue* jsonValue)
{
	ChineseCheckersGameClient::ProcessServerMessage(jsonValue);
}