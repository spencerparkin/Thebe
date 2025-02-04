#include "HostGameDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/msgdlg.h>

HostGameDialog::HostGameDialog(wxWindow* parent) : wxDialog(parent, wxID_ANY, "Host Game")
{
	wxStaticText* hostAddressLabel = new wxStaticText(this, wxID_ANY, "Host Address:");
	this->hostAddressText = new wxTextCtrl(this, wxID_ANY);
	this->hostAddressText->SetValue("localhost");

	wxStaticText* hostPortLabel = new wxStaticText(this, wxID_ANY, "Host Port:");
	this->hostPortText = new wxTextCtrl(this, wxID_ANY);
	this->hostPortText->SetValue("5050");

	wxStaticText* numHumanPlayersLabel = new wxStaticText(this, wxID_ANY, "Num. Human Players:");
	this->numHumanPlayersSpin = new wxSpinCtrl(this, wxID_ANY);
	this->numHumanPlayersSpin->SetValue(1);

	wxStaticText* numComputerPlayersLabel = new wxStaticText(this, wxID_ANY, "Num. Computer Players:");
	this->numComputerPlayersSpin = new wxSpinCtrl(this, wxID_ANY);
	this->numComputerPlayersSpin->SetValue(1);

	wxString gameTypeArray[] =
	{
		"traditional"
	};

	int gameTypeArraySize = sizeof(gameTypeArray) / sizeof(wxString);

	wxStaticText* gameTypeLabel = new wxStaticText(this, wxID_ANY, "Game Type:");
	this->gameTypeCombo = new wxComboBox(this, wxID_ANY, gameTypeArray[1], wxDefaultPosition, wxDefaultSize, gameTypeArraySize, gameTypeArray, wxCB_READONLY | wxCB_SORT);

	wxGridSizer* gridSizer = new wxGridSizer(2);
	gridSizer->SetRows(5);
	gridSizer->SetCols(2);
	gridSizer->Add(gameTypeLabel);
	gridSizer->Add(this->gameTypeCombo);
	gridSizer->Add(hostAddressLabel);
	gridSizer->Add(this->hostAddressText);
	gridSizer->Add(hostPortLabel);
	gridSizer->Add(this->hostPortText);
	gridSizer->Add(numHumanPlayersLabel);
	gridSizer->Add(this->numHumanPlayersSpin);
	gridSizer->Add(numComputerPlayersLabel);
	gridSizer->Add(this->numComputerPlayersSpin);
	
	wxButton* okayButton = new wxButton(this, wxID_ANY, "Okay");
	okayButton->Bind(wxEVT_BUTTON, &HostGameDialog::OnOkayButton, this);

	wxButton* cancelButton = new wxButton(this, wxID_ANY, "Cancel");
	cancelButton->Bind(wxEVT_BUTTON, &HostGameDialog::OnCancelButton, this);

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(cancelButton, 0, wxRIGHT, 10);
	buttonSizer->Add(okayButton);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(gridSizer, 1, wxGROW | wxALL, 10);
	mainSizer->Add(new wxStaticLine(this, wxID_ANY), 0);
	mainSizer->Add(buttonSizer, 0, wxGROW | wxLEFT | wxRIGHT | wxDOWN, 10);

	this->SetSizer(mainSizer);
}

/*virtual*/ HostGameDialog::~HostGameDialog()
{
}

const HostGameDialog::Data& HostGameDialog::GetData() const
{
	return this->data;
}

void HostGameDialog::OnOkayButton(wxCommandEvent& event)
{
	if (this->numHumanPlayersSpin->GetValue() == 0 && this->numComputerPlayersSpin->GetValue() == 0)
	{
		wxMessageBox(wxT("Can't do zero players."), wxT("Validation Error"), wxICON_ERROR | wxOK, this);
		return;
	}

	this->data.numHumanPlayers = this->numHumanPlayersSpin->GetValue();
	this->data.numComputerPlayers = this->numComputerPlayersSpin->GetValue();
	this->data.hostAddress.SetAddress((const char*)this->hostAddressText->GetValue().c_str());
	this->data.hostAddress.SetPort(::atoi((const char*)this->hostPortText->GetValue().c_str()));
	this->data.gameType = (const char*)this->gameTypeCombo->GetValue().c_str();

	this->EndModal(wxID_OK);
}

void HostGameDialog::OnCancelButton(wxCommandEvent& event)
{
	this->EndModal(wxID_CANCEL);
}