#pragma once
#include <QMainWindow>
#include <QImage>
#include <QJsonObject>
#include <QFutureWatcher>
#include "core/Recognizer.h"
class QLabel;class QPushButton;class QVBoxLayout;class QStackedWidget;class QScrollArea;class QDialog;class ImageCanvas;class ScoreCard;
class MainWindow:public QMainWindow {
public:
    explicit MainWindow(bool mobile=false);~MainWindow();
    bool importImage(const QString&path);bool loadConfig(const QString&path);bool saveResults(const QString&dir);
    QJsonObject resultJson()const;const markscan::Result&result()const{return result_;}
    void runSmoke(const QString&root);void runDemo();
private:
    enum class State{Empty,Busy,Ready,Failure};
    void chooseImage();void startImport(const QString&path);bool decodeImage(const QString&path,cv::Mat&bgr);
    void presentResult();void setFailure(const QString&message);void setState(State state);void updateActions();
    QWidget*buildEmpty();QWidget*buildBusy();QWidget*buildFailure();QWidget*buildResults();QWidget*buildPreview();
    void renderQuestions();void updatePreview();void selectView(int index);QImage currentImage()const;
    void showMore();void showSamples();void showTemplateHelp();void showExport();void showPreview();
    void saveAll();void exportPng(int index);void notify(const QString&message);
    bool mobile_;State state_=State::Empty;bool configured_=false,busy_=false,onlyReview_=false;
    int viewIndex_=0,shownQuestions_=0;
    QLabel*source_;QLabel*failureText_;QLabel*feedback_;QLabel*reviewHint_;QLabel*stats_[4];
    QPushButton*import_=nullptr;QPushButton*save_=nullptr;QPushButton*more_;QPushButton*filter_;QPushButton*segments_[3];
    ScoreCard*score_;ImageCanvas*canvas_;QVBoxLayout*questionLayout_;QStackedWidget*screens_;QScrollArea*resultsScroll_;
    QDialog*previewDialog_=nullptr;ImageCanvas*fullCanvas_=nullptr;
    markscan::Config config_;markscan::Result result_;QImage original_;QString path_,configPath_,error_;
    QFutureWatcher<markscan::Result>*watcher_;
};
