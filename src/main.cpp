#include "../inc/main.h"

bool Main::OnInit() {
    MyFrame* frame = new MyFrame();
    frame->Centre();
    frame->Show(true);
    return true;
}

wxIMPLEMENT_APP(Main);

MyFrame::MyFrame()
    : wxFrame(NULL, wxID_ANY, "Document Reader", wxDefaultPosition, wxSize(550, 300)) {

    SetBackgroundColour(wxColour(88, 96, 209));

    wxFont font(16, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxPanel* panelText = new wxPanel(this);
    wxPanel* panelButton = new wxPanel(this);

    panelText->SetFont(font);
    panelButton->SetFont(font);

	text = new wxStaticText(panelText, wxID_ANY,
		"Select a photo with the document\n to get a photo scan",
		wxDefaultPosition,
		wxSize(500, -1),
		wxALIGN_CENTER);

    text->SetForegroundColour(*wxWHITE);

    button = new wxButton(panelButton, wxID_ANY, "Choose file");
    button->Bind(wxEVT_BUTTON, &MyFrame::FileDialog, this);

	wxBoxSizer* s1 = new wxBoxSizer(wxVERTICAL);

	s1->AddStretchSpacer(1);
	s1->Add(text, 0, wxALIGN_CENTER | wxEXPAND);
	s1->AddStretchSpacer(1);

    panelText->SetSizer(s1);

    wxBoxSizer* s2 = new wxBoxSizer(wxVERTICAL);
    s2->Add(button, 0, wxALIGN_CENTER | wxALL, 10);

    panelButton->SetSizer(s2);

    mainSizer->Add(panelText, 1, wxEXPAND);
    mainSizer->Add(panelButton, 1, wxEXPAND);

    SetSizer(mainSizer);
}

string ProcessingThePath(wxString path) {
    string result = path.ToStdString();

    for (char& c : result) {
        if (c == '\\') c = '/';
    }

    return result;
}

void MyFrame::FileDialog(wxCommandEvent&) {

    wxFileDialog openFileDialog(this, "Open Image", "", "",
        "Image files (*.jpg;*.png,*.JPG)|*.jpg;*.png;*.JPG",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    string path = ProcessingThePath(openFileDialog.GetPath());

	wxArrayString choices;
	choices.Add("Original");
	choices.Add("Grayscale");
	choices.Add("Black & White");

	int choice = wxGetSingleChoiceIndex(
		"Select processing mode",
		"Mode",
		choices
	);

	if (choice == -1) return;

	ScanMode mode = ScanMode::ORIGINAL;

	if (choice == 1) mode = ScanMode::GRAYSCALE;
	if (choice == 2) mode = ScanMode::THRESHOLD;

    if (!ProcessDocument(path, mode)) {
        wxLogError("Failed to process image");
        text->SetLabelText("Try another image");
		text->Wrap(500); 
    }
    else {
        wxLogMessage("Saved in /results folder");
        text->SetLabelText("Done! Choose another image");
		text->Wrap(500); 
    }
}