#ifndef SCAN_H
#define SCAN_H

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <vector>
#include <fstream>
#include <filesystem>

#include "common.h"

using std::string;
using std::vector;
// ======================= IMAGE PROCESSING =======================

vector<cv::Point> Find_Contours(const cv::Mat& imgDil);

cv::Mat PreprocessingImage_toWarping(const cv::Mat& img);

vector<cv::Point> Reorder(const vector<cv::Point>& points);
cv::Mat getWarp(const cv::Mat& img, const vector<cv::Point>& pts);

vector<cv::Point> adjustPoints(const cv::Mat& img, const vector<cv::Point>& initial);

void drawUI(cv::Mat& img);

// ======================= CORE =======================

bool ProcessDocument(const string& path, ScanMode mode);

// ======================= MOUSE =======================

void mouseEvent(int event, int x, int y, int flags, void* param);

// ======================= UTILS =======================

int findClosestPoint(int x, int y);

vector<cv::Point> getDefaultQuad(const cv::Mat& img);

#endif