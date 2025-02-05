#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include "Thebe/Network/Address.h"

class JoinGameDialog : public wxDialog
{
public:
	JoinGameDialog(wxWindow* parent);
	virtual ~JoinGameDialog();

	struct Data
	{
		Thebe::NetworkAddress hostAddress;
	};

	const Data& GetData() const;

private:
	void OnOkayButton(wxCommandEvent& event);
	void OnCancelButton(wxCommandEvent& event);

	Data data;
	wxTextCtrl* hostAddressText;
	wxTextCtrl* hostPortText;
};