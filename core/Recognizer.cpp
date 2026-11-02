#include "core/Recognizer.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace markscan {
const char* statusName(Status s) {
    switch(s) { case Status::Correct:return "correct"; case Status::Wrong:return "wrong";
      case Status::Multiple:return "multiple"; default:return "unanswered"; }
}
namespace {
cv::Mat binary(const cv::Mat& image) {
    cv::Mat gray,blur,bin;
    cv::cvtColor(image,gray,cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray,blur,{3,3},0);
    cv::adaptiveThreshold(blur,bin,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY_INV,51,12);
    return bin;
}
double diskRatio(const cv::Mat& bin,cv::Point center,int radius) {
    const cv::Rect roi(center.x-radius,center.y-radius,2*radius+1,2*radius+1);
    cv::Mat mask(roi.size(),CV_8U,cv::Scalar(0)),inside;
    cv::circle(mask,{radius,radius},radius,255,-1);
    cv::bitwise_and(bin(roi),mask,inside);
    return double(cv::countNonZero(inside))/cv::countNonZero(mask);
}
bool templateValid(const cv::Mat& bin) {
    // Asymmetric 10110 orientation/template code; blank cells must remain white.
    const int bits[5]={1,0,1,1,0};
    for(int i=0;i<5;++i) {
        double r=diskRatio(bin,{420+i*70,210},6);
        if(bits[i] ? r<0.8 : r>0.1) return false;
    }
    int rings=0;
    for(int q=0;q<10;++q) for(int a=0;a<4;++a) {
        auto p=optionCenter(q,a);
        cv::Mat mask(61,61,CV_8U,cv::Scalar(0)),inside;
        cv::circle(mask,{30,30},25,255,5);
        cv::bitwise_and(bin(cv::Rect(p.x-30,p.y-30,61,61)),mask,inside);
        if(double(cv::countNonZero(inside))/cv::countNonZero(mask)>0.35) ++rings;
    }
    return rings>=36;
}
struct Quad { std::array<cv::Point2f,4> p; double area; };
}
Result recognize(const cv::Mat& input,const Config& config) {
    Result result;
    if(input.empty() || input.type()!=CV_8UC3 || input.cols<200 || input.rows<200) {
        result.error="Invalid image: provide a BGR image at least 200 x 200."; return result;
    }
    if(config.fillThreshold<=0 || config.fillThreshold>=1 || config.pointsPerQuestion!=10 ||
       std::any_of(config.answers.begin(),config.answers.end(),[](int a){return a<0||a>3;})) {
        result.error="Invalid answer configuration."; return result;
    }
    try {
        cv::Mat image=input;
        double scale=std::min(1.,2400./std::max(input.cols,input.rows));
        if(scale<1) cv::resize(input,image,{},scale,scale,cv::INTER_AREA);
        auto bin=binary(image);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(bin,contours,cv::RETR_LIST,cv::CHAIN_APPROX_SIMPLE);
        std::vector<std::pair<double,cv::Point2f>> markers;
        for(const auto& c:contours) {
            double area=std::abs(cv::contourArea(c));
            if(area<image.total()*0.00018 || area>image.total()*0.012) continue;
            std::vector<cv::Point> poly;
            cv::approxPolyDP(c,poly,0.035*cv::arcLength(c,true),true);
            if(poly.size()!=4 || !cv::isContourConvex(poly)) continue;
            auto rect=cv::minAreaRect(c);
            double aspect=rect.size.width/rect.size.height;
            if(aspect<0.6 || aspect>1.67 || area/rect.size.area()<0.82) continue;
            auto m=cv::moments(c);
            cv::Point2f p(float(m.m10/m.m00),float(m.m01/m.m00));
            bool duplicate=false;
            for(const auto& old:markers) if(cv::norm(old.second-p)<10) duplicate=true;
            if(!duplicate) markers.push_back({area,p});
        }
        std::sort(markers.begin(),markers.end(),[](const auto& a,const auto& b){return a.first>b.first;});
        if(markers.size()>20) markers.resize(20);
        std::vector<Quad> quads;
        int n=int(markers.size());
        for(int a=0;a<n;++a) for(int b=a+1;b<n;++b) for(int c=b+1;c<n;++c) for(int d=c+1;d<n;++d) {
            std::vector<cv::Point2f> p={markers[a].second,markers[b].second,markers[c].second,markers[d].second}, hull;
            cv::convexHull(p,hull);
            if(hull.size()!=4) continue;
            double area=std::abs(cv::contourArea(hull));
            if(area<image.total()*0.22) continue;
            Quad quad; quad.area=area;
            std::copy(hull.begin(),hull.end(),quad.p.begin());
            quads.push_back(quad);
        }
        std::sort(quads.begin(),quads.end(),[](const Quad&a,const Quad&b){return a.area>b.area;});
        auto dst=markerCenters();
        bool located=false;
        for(size_t i=0;i<std::min(size_t(60),quads.size()) && !located;++i) {
            for(int rotation=0;rotation<4;++rotation) {
                std::array<cv::Point2f,4> src;
                for(int k=0;k<4;++k) src[k]=quads[i].p[(k+rotation)%4];
                auto h=cv::getPerspectiveTransform(src.data(),dst.data());
                cv::Mat corrected;
                cv::warpPerspective(image,corrected,h,{Width,Height},cv::INTER_LINEAR,cv::BORDER_CONSTANT,{255,255,255});
                auto candidate=binary(corrected);
                if(templateValid(candidate)) { result.corrected=corrected; bin=candidate; located=true; break; }
            }
        }
        if(!located) {
            result.error="Card location failed: show all four black markers and use the MarkScan v1 template.";
            return result;
        }
        result.annotated=result.corrected.clone();
        for(int q=0;q<10;++q) {
            auto& item=result.questions[q];
            for(int a=0;a<4;++a) {
                item.ratios[a]=diskRatio(bin,optionCenter(q,a),InnerRadius);
                if(item.ratios[a]>=config.fillThreshold) item.selected+=char('A'+a);
            }
            if(item.selected.empty()) item.status=Status::Unanswered;
            else if(item.selected.size()>1) item.status=Status::Multiple;
            else if(item.selected[0]=='A'+config.answers[q]) {item.status=Status::Correct;item.points=config.pointsPerQuestion;}
            else item.status=Status::Wrong;
            result.score+=item.points;
            cv::Scalar color=item.status==Status::Correct?cv::Scalar(0,150,0):cv::Scalar(0,0,220);
            for(int a=0;a<4;++a) if(item.ratios[a]>=config.fillThreshold) {
                auto p=optionCenter(q,a); cv::rectangle(result.annotated,{p.x-32,p.y-32,64,64},color,3);
            }
            cv::putText(result.annotated,statusName(item.status),{970,428+110*q},cv::FONT_HERSHEY_SIMPLEX,0.5,color,1,cv::LINE_AA);
        }
        cv::putText(result.annotated,"Score: "+std::to_string(result.score)+" / 100",{370,1550},cv::FONT_HERSHEY_SIMPLEX,1.2,{0,100,200},3,cv::LINE_AA);
        result.success=true;
    } catch(const cv::Exception& e) {result.error=std::string("OpenCV processing failed: ")+e.what();}
    return result;
}
}
