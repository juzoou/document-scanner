#include <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/wfstream.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

using std::string;
using std::cout;
using std::endl;
using std::vector;

enum class ScanMode {
    ORIGINAL,
    GRAYSCALE,
    THRESHOLD
};

static double distance(cv::Point a, cv::Point b) {
    return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

// ======================= IMAGE PROCESSING =======================

vector<cv::Point> Find_Contours(const cv::Mat& imgDil) {
    vector<vector<cv::Point>> contours;
    cv::findContours(imgDil, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    vector<cv::Point> biggest;
    double maxArea = 0;

    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);

        const double MIN_AREA = 100;

        if (area > MIN_AREA) {
            double peri = cv::arcLength(contours[i], true);

            vector<cv::Point> approx;
            cv::approxPolyDP(contours[i], approx, 0.02 * peri, true);

            if (area > maxArea && approx.size() == 4 && cv::isContourConvex(approx)) {
                biggest = approx;
                maxArea = area;
            }
        }
    }

    return biggest;
}

cv::Mat PreprocessingImage_toWarping(const cv::Mat& img) {
    cv::Mat imgGray, imgBlur, imgCanny, imgDil;

    const int BLUR_SIZE = 13;
    const int CANNY_T1 = 50;
    const int CANNY_T2 = 150;

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(imgGray, imgBlur, cv::Size(BLUR_SIZE, BLUR_SIZE), 0);

    cv::Canny(imgBlur, imgCanny, CANNY_T1, CANNY_T2);
    cv::dilate(imgCanny, imgDil, kernel);

    return imgDil;
}

vector<cv::Point> Reorder(const vector<cv::Point>& points) {

    if (points.size() != 4) {
        throw std::runtime_error("Reorder: points != 4");
    }

    vector<cv::Point> reordered(4);
    vector<int> sum(4), diff(4);

    for (int i = 0; i < 4; i++) {
        sum[i] = points[i].x + points[i].y;
        diff[i] = points[i].x - points[i].y;
    }

    reordered[0] = points[std::min_element(sum.begin(), sum.end()) - sum.begin()]; // top-left
    reordered[3] = points[std::max_element(sum.begin(), sum.end()) - sum.begin()]; // bottom-right
    reordered[1] = points[std::max_element(diff.begin(), diff.end()) - diff.begin()]; // top-right
    reordered[2] = points[std::min_element(diff.begin(), diff.end()) - diff.begin()]; // bottom-left

    return reordered;
}

cv::Mat getWarp(const cv::Mat& img, const vector<cv::Point>& pts) {

    if (pts.size() != 4) {
        throw std::runtime_error("getWarp: invalid points");
    }

    float widthTop = distance(pts[0], pts[1]);
    float widthBottom = distance(pts[2], pts[3]);
    float heightLeft = distance(pts[0], pts[2]);
    float heightRight = distance(pts[1], pts[3]);

    float width = (widthTop + widthBottom) / 2.0f;
    float height = (heightLeft + heightRight) / 2.0f;

    const float MAX_RATIO = 1.5f;

    if (width / height > MAX_RATIO) {
        width = height * MAX_RATIO;
    }
    if (height / width > MAX_RATIO) {
        height = width * MAX_RATIO;
    }

    cv::Point2f src[4] = { pts[0], pts[1], pts[2], pts[3] };
    cv::Point2f dst[4] = {
        {0.0f, 0.0f},
        {width, 0.0f},
        {0.0f, height},
        {width, height}
    };

    cv::Mat matrix = cv::getPerspectiveTransform(src, dst);

    cv::Mat imgWarp;
    cv::warpPerspective(img, imgWarp, matrix, cv::Size(width, height));

    return imgWarp;
}

// ======================= CORE =======================

bool ProcessDocument(const string& path, ScanMode mode){

    cv::Mat img = cv::imread(path);

    if (img.empty()) {
        return false;
    }

    cv::Mat imgResized = img;
    if (img.cols > 1500) {
        cv::resize(img, imgResized, cv::Size(), 0.5, 0.5);
    }

    cv::Mat imgDil = PreprocessingImage_toWarping(imgResized);

    vector<cv::Point> contour = Find_Contours(imgDil);

    if (contour.empty()) {
        return false;
    }

    vector<cv::Point> ordered;
    try {
        ordered = Reorder(contour);
    } catch (...) {
        return false;
    }

    cv::Mat imgWarp;
    try {
        imgWarp = getWarp(imgResized, ordered);
    } catch (...) {
        return false;
    }

    if (imgWarp.empty()) {
        return false;
    }

	cv::Mat result = imgWarp;

	if (mode == ScanMode::GRAYSCALE) {
		cv::cvtColor(imgWarp, result, cv::COLOR_BGR2GRAY);
	}
	else if (mode == ScanMode::THRESHOLD) {
		cv::Mat gray;
		cv::cvtColor(imgWarp, gray, cv::COLOR_BGR2GRAY);

		cv::adaptiveThreshold(gray, result, 255,
			cv::ADAPTIVE_THRESH_MEAN_C,
			cv::THRESH_BINARY, 11, 2);
	}

    cv::Mat gray, thresh;
    cv::cvtColor(imgWarp, gray, cv::COLOR_BGR2GRAY);

    std::filesystem::path folder = std::filesystem::current_path() / "results";

    try {
        if (!std::filesystem::exists(folder)) {
            std::filesystem::create_directory(folder);
        }

        string filename = "result_" + std::to_string(std::time(nullptr)) + ".jpg";
        string fullpath = (folder / filename).string();

        return cv::imwrite(fullpath, result);

    } catch (...) {
        return false;
    }
}

// ======================= GUI =======================

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
        "Image files (*.jpg;*.png)|*.jpg;*.png",
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