#pragma once
#include <opencv2/core.hpp>
#include <string>
#include <vector>
cv::Mat makeSheet(const std::vector<std::string>& answers);
cv::Mat perspectiveSheet(const cv::Mat& input);
