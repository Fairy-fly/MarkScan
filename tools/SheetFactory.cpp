#include "tools/SheetFactory.h"
#include "core/Recognizer.h"
#include <opencv2/imgproc.hpp>
cv::Mat makeSheet(const std::vector<std::string>& answers) {
    using namespace markscan;
    cv::Mat image(Height,Width,CV_8UC3,cv::Scalar(255,255,255));
    for(auto p:markerCenters()) cv::rectangle(image,{int(p.x)-22,int(p.y)-22,45,45},{0,0,0},-1);
    auto text=[&](std::string s,cv::Point p,double size=0.8,int weight=2){cv::putText(image,s,p,cv::FONT_HERSHEY_SIMPLEX,size,{0,0,0},weight,cv::LINE_AA);};
    text("MarkScan",{410,130},1.8,3);
    int bits[5]={1,0,1,1,0};
    for(int i=0;i<5;++i) if(bits[i]) cv::rectangle(image,{410+i*70,200,21,21},{0,0,0},-1);
    text("ANSWER SHEET v1  |  10 questions x 10 points",{195,280},0.8);
    text("Name: __________________  ID: __________________",{190,330},0.7);
    for(int a=0;a<4;++a) text(std::string(1,char('A'+a)),{427+160*a,375},0.9);
    for(int q=0;q<10;++q) {
        text(std::to_string(q+1),{270,430+110*q},0.9);
        for(int a=0;a<4;++a) {
            auto p=optionCenter(q,a); cv::circle(image,p,Radius,{0,0,0},2,cv::LINE_AA);
            if(q<int(answers.size()) && answers[q].find(char('A'+a))!=std::string::npos)
                cv::circle(image,p,21,{0,0,0},-1,cv::LINE_AA);
        }
    }
    text("Fill ONE circle per row completely with a dark pencil.",{180,1590},0.7);
    text("Keep all 4 square markers visible. Print A4 portrait, 100%.",{180,1630},0.65);
    return image;
}
cv::Mat perspectiveSheet(const cv::Mat& input) {
    cv::Mat output(2000,1600,CV_8UC3,cv::Scalar(95,100,110));
    cv::Point2f src[]={{0,0},{1239,0},{1239,1753},{0,1753}};
    cv::Point2f dst[]={{200,160},{1370,70},{1450,1800},{85,1900}};
    cv::warpPerspective(input,output,cv::getPerspectiveTransform(src,dst),output.size(),cv::INTER_LINEAR,cv::BORDER_TRANSPARENT);
    return output;
}
