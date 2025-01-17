#pragma once

#include "GameClient.h"
#include <wx/progdlg.h>

/**
 *
 */
class HumanClient : public ChineseCheckersClient
{
public:
	HumanClient();
	virtual ~HumanClient();

	virtual void ProcessServerMessage(const ParseParty::JsonValue* jsonValue) override;
	virtual void HandleConnectionStatus(ConnectionStatus status, int i, bool* abort) override;

	void TakeTurn(const std::vector<ChineseCheckersGame::Node*>& nodeArray);

	void SnapCubiesIntoPosition();

private:
	bool animate;
	wxProgressDialog* connectionProgressDialog;
};