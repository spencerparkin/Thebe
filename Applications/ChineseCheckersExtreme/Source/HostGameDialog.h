#pragma once

#include <wx/dialog.h>
#include <wx/textentry.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include "Thebe/Network/Address.h"

class HostGameDialog : public wxDialog
{
public:
	HostGameDialog(wxWindow* parent);
	virtual ~HostGameDialog();

	struct Data
	{
		int maxPlayers;
		int numAIPlayers;
		std::string gameType;
		Thebe::NetworkAddress hostAddress;
	};

	const Data& GetData() const;

private:
	void OnOkayButton(wxCommandEvent& event);
	void OnCancelButton(wxCommandEvent& event);

	Data data;
	wxTextCtrl* hostAddressText;
	wxTextCtrl* hostPortText;
	wxSpinCtrl* numPlayersSpin;
	wxSpinCtrl* numAIPlayersSpin;
	wxComboBox* gameTypeCombo;
};