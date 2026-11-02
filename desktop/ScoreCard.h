#pragma once
#include <QWidget>
#include "core/Recognizer.h"
class ScoreCard:public QWidget {
public:
    explicit ScoreCard(QWidget*parent=nullptr);
    void setResult(const markscan::Result& result);
protected:void paintEvent(QPaintEvent*)override;
private:int score_=0,correct_=0;
};
