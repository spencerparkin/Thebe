#include "HostGameDialog.h"
#include "Game.h"
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

	wxStaticText* numPlayersLabel = new wxStaticText(this, wxID_ANY, "Num. Players:");
	this->numPlayersSpin = new wxSpinCtrl(this, wxID_ANY);
	this->numPlayersSpin->SetValue(2);

	wxStaticText* numAIPlayersLabel = new wxStaticText(this, wxID_ANY, "Num. AI Players:");
	this->numAIPlayersSpin = new wxSpinCtrl(this, wxID_ANY);
	this->numAIPlayersSpin->SetValue(1);

	wxString gameTypeArray[] =
	{
		"cubic",
		"hexagonal",
		"octagonal",
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
	gridSizer->Add(numPlayersLabel);
	gridSizer->Add(this->numPlayersSpin);
	gridSizer->Add(numAIPlayersLabel);
	gridSizer->Add(this->numAIPlayersSpin);
	
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
	wxString gameType = this->gameTypeCombo->GetValue();
	std::unique_ptr<ChineseCheckersGame> game(ChineseCheckersGame::Factory((const char*)gameType.c_str()));
	THEBE_ASSERT_FATAL(game != nullptr);

	// TODO: Can we just add validators to the controls so that it's not possible to choose a wrong value?

	if (this->numPlayersSpin->GetValue() > game->GetMaxPossiblePlayers())
	{
		wxMessageBox(wxString::Format("The \"%s\" game type can only support up to %d players.", (const char*)gameType.c_str(), game->GetMaxPossiblePlayers()), "Validation Error", wxICON_ERROR | wxOK, this);
		return;
	}

	if (this->numPlayersSpin->GetValue() <= 1)
	{
		wxMessageBox("The number of players must be greater than one.", "Validation Error", wxICON_ERROR | wxOK, this);
		return;
	}

	if (this->numAIPlayersSpin->GetValue() > this->numPlayersSpin->GetValue())
	{
		wxMessageBox("The number of AI players can't exceed the number of players.", "Validation Error", wxICON_ERROR | wxOK, this);
		return;
	}

	this->data.numPlayers = this->numPlayersSpin->GetValue();
	this->data.numAIPlayers = this->numAIPlayersSpin->GetValue();
	this->data.hostAddress.SetAddress((const char*)this->hostAddressText->GetValue().c_str());
	this->data.hostAddress.SetPort(::atoi((const char*)this->hostPortText->GetValue().c_str()));
	this->data.gameType = (const char*)gameType.c_str();

	this->EndModal(wxID_OK);
}

void HostGameDialog::OnCancelButton(wxCommandEvent& event)
{
	this->EndModal(wxID_CANCEL);
}