#include "JoinGameDialog.h"
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/button.h>

JoinGameDialog::JoinGameDialog(wxWindow* parent) : wxDialog(parent, wxID_ANY, "Join Game")
{
	wxStaticText* hostAddressLabel = new wxStaticText(this, wxID_ANY, "Host Address:");
	this->hostAddressText = new wxTextCtrl(this, wxID_ANY);
	this->hostAddressText->SetValue("localhost");

	wxStaticText* hostPortLabel = new wxStaticText(this, wxID_ANY, "Host Port:");
	this->hostPortText = new wxTextCtrl(this, wxID_ANY);
	this->hostPortText->SetValue("5050");

	wxGridSizer* gridSizer = new wxGridSizer(2);
	gridSizer->SetRows(5);
	gridSizer->SetCols(2);
	gridSizer->Add(hostAddressLabel);
	gridSizer->Add(this->hostAddressText);
	gridSizer->Add(hostPortLabel);
	gridSizer->Add(this->hostPortText);

	wxButton* okayButton = new wxButton(this, wxID_ANY, "Okay");
	okayButton->Bind(wxEVT_BUTTON, &JoinGameDialog::OnOkayButton, this);

	wxButton* cancelButton = new wxButton(this, wxID_ANY, "Cancel");
	cancelButton->Bind(wxEVT_BUTTON, &JoinGameDialog::OnCancelButton, this);

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

/*virtual*/ JoinGameDialog::~JoinGameDialog()
{
}

const JoinGameDialog::Data& JoinGameDialog::GetData() const
{
	return this->data;
}

void JoinGameDialog::OnOkayButton(wxCommandEvent& event)
{
	this->data.hostAddress.SetAddress((const char*)this->hostAddressText->GetValue().c_str());
	this->data.hostAddress.SetPort(::atoi((const char*)this->hostPortText->GetValue().c_str()));

	this->EndModal(wxID_OK);
}

void JoinGameDialog::OnCancelButton(wxCommandEvent& event)
{
	this->EndModal(wxID_CANCEL);
}