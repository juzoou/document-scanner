#ifndef MAIN_H
#define MAIN_H

#include <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/wfstream.h>

#include <string>

#include "common.h"
#include "scan.h"

using std::string;

string ProcessingThePath(wxString path);


class MyFrame : public wxFrame {
public:
    MyFrame();

private:
    void FileDialog(wxCommandEvent& event);

    wxButton* button;
    wxStaticText* text;
};

class Main : public wxApp {
public:
    virtual bool OnInit();
};

#endif