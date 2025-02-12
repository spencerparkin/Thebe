#pragma once

#include "GameClient.h"
#include "ChineseCheckers/Graph.h"
#include "Thebe/Math/Vector4.h"
#include "Animation.h"
#include <wx/string.h>

class HumanClient : public ChineseCheckersGameClient
{
public:
	HumanClient();
	virtual ~HumanClient();

	virtual void Shutdown() override;
	virtual void Update(double deltaTimeSeconds) override;
	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;

private:
	void RegenerateScene();

	static Thebe::Vector4 MarbleColor(ChineseCheckers::Marble::Color color, double alpha);
	static wxString MarbleText(ChineseCheckers::Marble::Color color);

	AnimationProcessor animationProcessor;
};