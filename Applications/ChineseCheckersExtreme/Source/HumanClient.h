#pragma once

#include "GameClient.h"
#include "ChineseCheckers/Graph.h"
#include "Thebe/Math/Vector4.h"
#include "MoveSequence.h"

class HumanClient : public ChineseCheckersGameClient
{
public:
	HumanClient();
	virtual ~HumanClient();

	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;

private:
	void RegenerateScene();
	void SnapCubiesIntoPosition();

	static Thebe::Vector4 MarbleColor(ChineseCheckers::Marble::Color color, double alpha);
};