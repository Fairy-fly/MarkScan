#include "core/Recognizer.h"
#include "tools/SheetFactory.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace markscan;
namespace fs=std::filesystem;
int main(int argc,char**argv) {
    fs::path root=argc>1?argv[1]:".";
    fs::create_directories(root/"tests/images");fs::create_directories(root/"tests/results");
    cv::imwrite((root/"templates/blank.png").string(),makeSheet({}));
    std::vector<std::string> correct={"A","B","C","D","A","C","B","D","C","A"};
    struct Case {std::string name;cv::Mat image;std::vector<std::string> expected;int score;bool success;};
    std::vector<Case> cases;
    cases.push_back({"all_correct",makeSheet(correct),correct,100,true});
    auto wrong=correct;wrong[1]="D";wrong[4]="B";wrong[9]="C";
    cases.push_back({"partial_wrong",makeSheet(wrong),wrong,70,true});
    auto blank=correct;blank[2]="";blank[7]="";
    cases.push_back({"unanswered",makeSheet(blank),blank,80,true});
    auto multiple=correct;multiple[3]="BD";
    cases.push_back({"multiple",makeSheet(multiple),multiple,90,true});
    cases.push_back({"perspective",perspectiveSheet(makeSheet(correct)),correct,100,true});
    cv::Mat rotated;cv::rotate(makeSheet(correct),rotated,cv::ROTATE_90_CLOCKWISE);
    cases.push_back({"rotated_90",rotated,correct,100,true});
    cases.push_back({"blank_sheet",makeSheet({}),std::vector<std::string>(10),0,true});
    cv::Mat noCard(1000,800,CV_8UC3,cv::Scalar(210,220,230));
    cv::putText(noCard,"NO ANSWER CARD",{100,500},cv::FONT_HERSHEY_SIMPLEX,1,{30,30,30},2);
    cases.push_back({"no_card",noCard,{},0,false});
    auto missing=makeSheet(correct);cv::rectangle(missing,{50,50,60,60},{255,255,255},-1);
    cases.push_back({"missing_marker",missing,{},0,false});
    auto impostor=makeSheet({});cv::rectangle(impostor,{150,150,950,1450},{255,255,255},-1);
    cases.push_back({"four_squares_only",impostor,{},0,false});
    cv::Mat dark=perspectiveSheet(makeSheet(correct));
    for(int y=0;y<dark.rows;++y) for(int x=0;x<dark.cols;++x) {
        auto& p=dark.at<cv::Vec3b>(y,x);double gain=0.48+0.45*x/dark.cols;
        for(int c=0;c<3;++c)p[c]=cv::saturate_cast<uchar>(p[c]*gain);
    }
    cv::GaussianBlur(dark,dark,{3,3},0);
    cases.push_back({"shadow_perspective",dark,correct,100,true});
    std::ofstream log(root/"tests/results/results.json");log<<"{\"cases\":[\n";
    int passed=0,matched=0,total=0;
    for(size_t c=0;c<cases.size();++c) {
        const auto& test=cases[c];auto path=root/"tests/images"/(test.name+".png");cv::imwrite(path.string(),test.image);
        // Decode saved pixels again. Core receives pixels only.
        auto pixels=cv::imread(path.string());auto before=pixels.clone();auto r=recognize(pixels);
        bool pass=r.success==test.success && cv::norm(before,pixels,cv::NORM_INF)==0;
        if(test.success) {
            pass=pass && r.score==test.score;
            for(int q=0;q<10;++q) {
                ++total;bool ok=r.success && r.questions[q].selected==test.expected[q];if(ok)++matched;else pass=false;
                Status expected=test.expected[q].empty()?Status::Unanswered:test.expected[q].size()>1?Status::Multiple:
                    test.expected[q]==correct[q]?Status::Correct:Status::Wrong;
                if(r.questions[q].status!=expected) pass=false;
            }
        }
        if(r.success) {cv::imwrite((root/"tests/results"/(test.name+"_annotated.png")).string(),r.annotated);}
        if(pass)++passed;
        std::cout<<test.name<<": "<<(pass?"PASS":"FAIL")<<" located="<<r.success<<" score="<<r.score<<" expected="<<test.score<<" "<<r.error<<"\n";
        log<<(c?",\n":"")<<"{\"name\":\""<<test.name<<"\",\"pass\":"<<(pass?"true":"false")<<",\"located\":"<<(r.success?"true":"false")<<",\"score\":"<<r.score<<",\"expected_score\":"<<test.score<<",\"answers\":[";
        for(int q=0;q<10;++q)log<<(q?",":"")<<"\""<<r.questions[q].selected<<"\"";
        log<<"]}";
    }
    // Metamorphic checks: changing the key affects scoring, never pixel interpretation.
    Config alternate;alternate.answers.fill(3);auto changed=recognize(makeSheet(correct),alternate);
    bool keyOk=changed.success && changed.score==20;
    for(int q=0;q<10;++q)keyOk=keyOk && changed.questions[q].selected==correct[q];
    bool emptyOk=!recognize(cv::Mat()).success;
    bool configOk;Config bad;bad.fillThreshold=2;configOk=!recognize(makeSheet(correct),bad).success;
    std::cout<<"alternate_key: "<<(keyOk?"PASS":"FAIL")<<"\nempty_input: "<<(emptyOk?"PASS":"FAIL")<<"\ninvalid_config: "<<(configOk?"PASS":"FAIL")<<"\n";
    passed+=keyOk+emptyOk+configOk;int checks=int(cases.size())+3;
    log<<"\n],\"checks_passed\":"<<passed<<",\"checks_total\":"<<checks<<",\"question_matches\":"<<matched<<",\"question_total\":"<<total<<",\"scope\":\"synthetic fixed-template samples only; not real-photo accuracy\"}\n";
    std::cout<<"Checks "<<passed<<"/"<<checks<<"; synthetic question matches "<<matched<<"/"<<total<<"\n";
    return passed==checks?0:1;
}
