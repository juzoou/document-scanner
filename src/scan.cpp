#include "../inc/scan.h"

#define RADIUS 20

static std::vector<cv::Point> points;
static int selected_point = -1;
static float displayScale = 1.0f;

// ======================= UTILS=======================

static float distance(cv::Point a, cv::Point b) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return std::sqrtf(static_cast<float>(dx*dx + dy*dy));
}

static cv::Point scalePoint(int x, int y) {
    return cv::Point(
        static_cast<int>(x / displayScale),
        static_cast<int>(y / displayScale)
    );
}

static cv::Mat getDisplayImage(const cv::Mat& img) {
    int maxWidth = 1000;
    int maxHeight = 1200;

    float scaleX = (float)maxWidth / img.cols;
    float scaleY = (float)maxHeight / img.rows;

    float scale = std::min(scaleX, scaleY);
    displayScale = (scale < 1.0f) ? scale : 1.0f;

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(), displayScale, displayScale);

    return resized;
}

static void putTextInBox(cv::Mat& img, const std::string& text, cv::Point pos, 
                  int fontFace, double fontScale, cv::Scalar textColor, 
                  int thickness, cv::Scalar boxColor) {
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
    baseline += thickness;

    cv::Rect textBox(pos.x, pos.y - textSize.height, textSize.width, static_cast<int>(textSize.height) + static_cast<int>(baseline));

    cv::rectangle(img, textBox, boxColor, cv::FILLED);

    cv::putText(img, text, pos, fontFace, fontScale, textColor, thickness, cv::LINE_AA);
}

// ======================= IMAGE PROCESSING =======================

std::vector<cv::Point> Find_Contours(const cv::Mat& imgDil) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(imgDil, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Point> biggest;
    double maxArea = 0;

    for (auto& cnt : contours) {
        double area = cv::contourArea(cnt);

        double totalArea = imgDil.rows * imgDil.cols;
        double minArea = 0.1 * totalArea;

        if (area > minArea) {
            double peri = cv::arcLength(cnt, true);

            std::vector<cv::Point> approx;
            cv::approxPolyDP(cnt, approx, 0.02 * peri, true);

            if (area > maxArea && approx.size() == 4 && cv::isContourConvex(approx)) {
                biggest = approx;
                maxArea = area;
            }
        }
    }

    return biggest;
}

cv::Mat PreprocessingImage_toWarping(const cv::Mat& img) {
    cv::Mat gray, blur, canny, dil;

    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blur, cv::Size(5,5), 0);
    cv::Canny(blur, canny, 30, 100);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, {3,3});
    cv::dilate(canny, dil, kernel);

    return dil;
}

std::vector<cv::Point> Reorder(const std::vector<cv::Point>& pts) {

    std::vector<cv::Point> r(4);
    std::vector<int> sum(4), diff(4);

    for (int i = 0; i < 4; i++) {
        sum[i] = pts[i].x + pts[i].y;
        diff[i] = pts[i].x - pts[i].y;
    }

    r[0] = pts[std::min_element(sum.begin(), sum.end()) - sum.begin()];
    r[3] = pts[std::max_element(sum.begin(), sum.end()) - sum.begin()];
    r[1] = pts[std::max_element(diff.begin(), diff.end()) - diff.begin()];
    r[2] = pts[std::min_element(diff.begin(), diff.end()) - diff.begin()];

    return r;
}

cv::Mat getWarp(const cv::Mat& img, const std::vector<cv::Point>& pts) {

    float w = (distance(pts[0], pts[1]) + distance(pts[2], pts[3])) / 2;
    float h = (distance(pts[0], pts[2]) + distance(pts[1], pts[3])) / 2;

    int iw = static_cast<int>(w);
    int ih = static_cast<int>(h);

    cv::Point2f src[4] = { pts[0], pts[1], pts[2], pts[3] };
    cv::Point2f dst[4] = { {0.f, 0.f}, {w,0.f}, {0.f,h}, {w,h} };

    cv::Mat m = cv::getPerspectiveTransform(src, dst);

    cv::Mat warp;
    cv::warpPerspective(img, warp, m, cv::Size(iw, ih));

    return warp;
}

// ======================= INTERACTION =======================

int findClosestPoint(int x, int y) {
    for (int i = 0; i < points.size(); i++) {
        if (distance(points[i], {x,y}) < RADIUS)
            return i;
    }
    return -1;
}

void mouseEvent(int event, int x, int y, int, void*) {

    cv::Point real = scalePoint(x, y);

    if (event == cv::EVENT_LBUTTONDOWN)
        selected_point = findClosestPoint(real.x, real.y);

    else if (event == cv::EVENT_MOUSEMOVE && selected_point != -1)
        points[selected_point] = real;

    else if (event == cv::EVENT_LBUTTONUP)
        selected_point = -1;
}

void drawUI(cv::Mat& img) {

    if (points.size() == 4) {
        for (int i = 0; i < 4; i++) {
            cv::Point p1(static_cast<int>(points[i].x * displayScale),
                         static_cast<int>(points[i].y * displayScale));
            cv::Point p2(static_cast<int>(points[(i+1)%4].x * displayScale),
                         static_cast<int>(points[(i+1)%4].y * displayScale));
            cv::line(img, p1, p2, {0,255,0}, 2);
        }
    }

    for (int i = 0; i < points.size(); i++) {
        cv::Point p(static_cast<int>(points[i].x * displayScale),
                    static_cast<int>(points[i].y * displayScale));

        cv::Scalar color = (i == selected_point)
            ? cv::Scalar(0,255,255)
            : cv::Scalar(0,0,255);

        cv::circle(img, p, 8, color, -1);
    }

    putTextInBox(img, "Drag corners. ENTER=OK ESC=Cancel",
        {20,30}, cv::FONT_HERSHEY_PLAIN, 2, {255,255,255}, 1, {0,0,0});
}

std::vector<cv::Point> adjustPoints(const cv::Mat& img, const std::vector<cv::Point>& initial){

    points = initial;

    cv::destroyAllWindows();
    cv::namedWindow("Adjust Document", cv::WINDOW_NORMAL);
    cv::setMouseCallback("Adjust Document", mouseEvent);

    while(true){

        if (cv::getWindowProperty("Adjust Document", cv::WND_PROP_VISIBLE) < 1) {
            points.clear();
            return points;
        }

        cv::Mat display = getDisplayImage(img);
        drawUI(display);
        cv::imshow("Adjust Document", display);

        int key = cv::waitKey(1);

        if (key == 13) break;
        if (key == 27) {
            points.clear();
            break;
        }
    }

    cv::destroyWindow("Adjust Document");
    return points;
}

// ======================= DEFAULT =======================

std::vector<cv::Point> getDefaultQuad(const cv::Mat& img) {
    int w = img.cols;
    int h = img.rows;
    int m = 50;

    return {
        {m,m},
        {w-m,m},
        {w-m,h-m},
        {m,h-m}
    };
}

// ======================= CORE =======================

bool ProcessDocument(const std::string& path, ScanMode mode){

    cv::Mat img = cv::imread(path);
    if (img.empty()) return false;

    cv::Mat resized = img;
    if (img.cols > 1500)
        cv::resize(img, resized, {}, 0.5, 0.5);

    cv::Mat prep = PreprocessingImage_toWarping(resized);

    auto contour = Find_Contours(prep);

    std::vector<cv::Point> initial =
        (contour.size() == 4) ? contour : getDefaultQuad(resized);

    auto adjusted = adjustPoints(resized, initial);
    if (adjusted.size() != 4) return false;

    auto ordered = Reorder(adjusted);
    cv::Mat warp = getWarp(resized, ordered);

    if (warp.empty()) return false;

    cv::Mat result = warp;

    if (mode == ScanMode::GRAYSCALE)
        cv::cvtColor(warp, result, cv::COLOR_BGR2GRAY);

    else if (mode == ScanMode::THRESHOLD) {
        cv::Mat gray;
        cv::cvtColor(warp, gray, cv::COLOR_BGR2GRAY);

        cv::adaptiveThreshold(gray, result, 255,
            cv::ADAPTIVE_THRESH_MEAN_C,
            cv::THRESH_BINARY, 11, 2);
    }

    std::filesystem::create_directory("results");

    std::string name = "results/result_" +
        std::to_string(std::time(nullptr)) + ".jpg";

    return cv::imwrite(name, result);
}