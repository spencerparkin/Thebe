#pragma once

#include "GameClient.h"
#include "ChineseCheckers/Graph.h"
#include "Thebe/Math/Vector4.h"
#include "Thebe/AudioSystem.h"
#include "Thebe/Math/Random.h"
#include "Thebe/EventSystem.h"
#include "Factory.h"
#include "Animation.h"
#include <wx/string.h>
#include <wx/progdlg.h>

class HumanClient : public ChineseCheckersGameClient
{
public:
	HumanClient();
	virtual ~HumanClient();

	virtual bool Setup() override;
	virtual void Shutdown() override;
	virtual void Update(double deltaTimeSeconds) override;
	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;
	virtual void HandleConnectionStatus(ConnectionStatus status, int i, bool* abort) override;

private:
	void RegenerateScene(const std::string& gameType);

	void HandleAudioEvent(const Thebe::AudioEvent* audioEvent);
	void HandleCollisionObjectEvent(const Thebe::Event* event);

	void QueueUpSongs();

	static Thebe::Vector4 MarbleColor(ChineseCheckers::Marble::Color color, double alpha);
	static wxString MarbleText(ChineseCheckers::Marble::Color color);

	Marble* FindMarbleOwningCollisionHandle(Thebe::RefHandle handle, Node*& node);

	AnimationProcessor animationProcessor;
	wxProgressDialog* connectionProgressDialog;
	Thebe::EventHandlerID audioEventHandlerID;
	Thebe::EventHandlerID collisionEventHandlerID;
	Thebe::Random random;
};