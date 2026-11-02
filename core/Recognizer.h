#pragma once
#include <array>
#include <string>
#include <vector>
#include <opencv2/core.hpp>

namespace markscan {
constexpr int Width=1240, Height=1754, Radius=25, InnerRadius=17;
inline cv::Point optionCenter(int q,int a) { return {440+160*a,420+110*q}; }
inline std::array<cv::Point2f,4> markerCenters() { return {{{80,80},{1160,80},{1160,1674},{80,1674}}}; }
struct Config {
    std::array<int,10> answers{{0,1,2,3,0,2,1,3,2,0}};
    double fillThreshold=0.40;
    int pointsPerQuestion=10;
};
enum class Status { Correct, Wrong, Unanswered, Multiple };
const char* statusName(Status s);
struct Question {
    std::array<double,4> ratios{};
    std::string selected;
    Status status=Status::Unanswered;
    int points=0;
};
struct Result {
    bool success=false;
    std::string error;
    int score=0;
    std::array<Question,10> questions;
    cv::Mat corrected, annotated;
};
// No Qt, filenames or test expectations enter the recognition API.
Result recognize(const cv::Mat& bgr,const Config& config=Config{});
}
