#include "desktop/ScoreCard.h"
#include "desktop/Theme.h"
#include <QPainter>
ScoreCard::ScoreCard(QWidget*p):QWidget(p){setMinimumHeight(170);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);setObjectName("scoreCard");}
void ScoreCard::setResult(const markscan::Result&r){score_=r.score;correct_=0;for(auto&q:r.questions)correct_+=q.status==markscan::Status::Correct;setAccessibleName(QString("%1 / 100 分，答对 %2 题，共10题").arg(score_).arg(correct_));update();}
void ScoreCard::paintEvent(QPaintEvent*) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);p.setBrush(QColor(theme::Ink));p.drawRoundedRect(rect(),12,12);
    QFont f=font();f.setPixelSize(13);p.setFont(f);p.setPen(QColor("#BEC2CF"));p.drawText(24,31,"本次得分");
    f.setPixelSize(width()<320?56:66);f.setWeight(QFont::DemiBold);p.setFont(f);p.setPen(QColor("#F8F7F2"));p.drawText(20,104,QString::number(score_));
    int textWidth=QFontMetrics(f).horizontalAdvance(QString::number(score_));f.setPixelSize(width()<320?12:16);f.setWeight(QFont::Normal);p.setFont(f);p.setPen(QColor("#B9BECA"));p.drawText(28+textWidth,100,"/ 100");
    f.setPixelSize(12);p.setFont(f);p.drawText(24,143,QString("答对 %1 题，共 10 题").arg(correct_));
    const int cx=width()-68,cy=79,r=36;
    p.setPen(QPen(QColor("#444A59"),5));p.drawEllipse(QPoint(cx,cy),r,r);
    p.setPen(QPen(QColor("#B9C2E8"),5,Qt::SolidLine,Qt::RoundCap));p.drawArc(QRect(cx-r,cy-r,2*r,2*r),90*16,-int(correct_*36)*16);
    f.setPixelSize(20);f.setWeight(QFont::DemiBold);p.setFont(f);p.setPen(Qt::white);p.drawText(QRect(cx-40,cy-16,80,32),Qt::AlignCenter,QString::number(correct_*10)+"%");
    f.setPixelSize(11);f.setWeight(QFont::Normal);p.setFont(f);p.setPen(QColor("#BEC2CF"));p.drawText(QRect(cx-54,128,108,20),Qt::AlignCenter,"答对题占比");
}
