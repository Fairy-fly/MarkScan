#include "desktop/Theme.h"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
namespace theme {
QIcon icon(const QString& name,const QColor& color) {
    QPixmap pix(72,72);pix.fill(Qt::transparent);QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);p.scale(3,3);
    p.setPen(QPen(color,1.65,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    if(name=="plus"){p.drawLine(12,5,12,19);p.drawLine(5,12,19,12);}
    else if(name=="more"){p.setBrush(color);for(int x:{5,12,19})p.drawEllipse(QPointF(x,12),1.2,1.2);}
    else if(name=="export"){p.drawLine(12,3,12,15);p.drawLine(8,7,12,3);p.drawLine(16,7,12,3);QPainterPath a;a.moveTo(5,13);a.lineTo(5,20);a.lineTo(19,20);a.lineTo(19,13);p.drawPath(a);}
    else if(name=="close"){p.drawLine(6,6,18,18);p.drawLine(18,6,6,18);}
    else if(name=="expand"){for(int k=0;k<4;++k){p.save();p.translate(12,12);p.rotate(k*90);p.drawLine(3,-8,8,-8);p.drawLine(8,-8,8,-3);p.restore();}}
    else if(name=="brand"){p.drawRoundedRect(QRectF(4,3,16,18),3,3);p.drawLine(7,12,10,15);p.drawLine(10,15,17,8);}
    return QIcon(pix);
}
void apply(QApplication& app,bool mobile) {
    app.setStyle("Fusion");
    QFont font=app.font();
#ifndef Q_OS_ANDROID
    font.setFamily("Microsoft YaHei UI");
#endif
    font.setPixelSize(mobile?15:14);app.setFont(font);
    app.setStyleSheet(QString(R"(
      QWidget { color:%1; }
      QMainWindow, QDialog { background:%2; }
      QWidget#surface { background:%3; }
      QLabel { background:transparent; }
      QLabel[role="muted"] { color:%4; font-size:12px; }
      QLabel[role="heading"] { font-size:18px; font-weight:600; }
      QLabel[role="brand"] { font-size:21px; font-weight:700; letter-spacing:-0.4px; }
      QLabel[role="hero"] { font-size:30px; font-weight:700; }
      QLabel[role="section"] {font-size:16px;font-weight:600;}
      QPushButton { background:%3; border:1px solid %5; border-radius:8px; padding:0 14px; min-height:46px; font-weight:500; }
      QPushButton:hover {background:#EAECEB;}
      QPushButton:pressed {background:#DDE0E6;}
      QPushButton:focus {border:1px solid %6;}
      QPushButton:disabled { color:#A6A8AE; background:#E8E8E3; border-color:#E8E8E3; }
      QPushButton[role="primary"] { background:%6; border-color:%6; color:white; font-weight:600; }
      QPushButton[role="primary"]:hover {background:#464F73;}
      QPushButton[role="primary"]:pressed {background:#394260;}
      QPushButton[role="primary"]:disabled {background:#B4B8C7;border-color:#B4B8C7;color:#F5F5F5;}
      QPushButton[role="quiet"] { background:transparent;border-color:transparent;color:%6; }
      QPushButton[role="quiet"]:hover {background:#E8E9E7;}
      QPushButton[role="segment"] {background:transparent;border-color:transparent;border-radius:6px;min-height:46px;padding:0 9px;color:%4;}
      QPushButton[role="segment"]:checked {background:white;color:%1;font-weight:600;}
      QPushButton[role="segment"]:disabled {color:#A6A8AE;}
      QPushButton[role="question"] {border:none;border-bottom:1px solid %5;border-radius:0;padding:8px 0;background:transparent;min-height:54px;}
      QPushButton[role="question"]:hover {background:#EAECE7;}
      QLabel[role="answer"] {font-size:16px;font-weight:600;}
      QFrame#segments {background:#E6E7E2;border-radius:8px;}
      QFrame#divider {background:%5;max-height:1px;border:none;}
      QScrollArea {background:transparent;border:none;}
      QScrollArea > QWidget > QWidget {background:transparent;}
      QScrollBar:vertical {background:transparent;width:6px;margin:2px;}
      QScrollBar::handle:vertical {background:#C8CAC6;border-radius:2px;min-height:30px;}
      QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical {height:0px;}
      QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical {background:transparent;}
      QMenu {background:white;border:1px solid %5;border-radius:8px;padding:6px;}
      QMenu::item {padding:14px 24px;min-width:180px;}
      QMenu::item:selected {background:#EFF0F5;color:%6;border-radius:5px;}
      QMenu::item:disabled {color:#AAADB5;}
      QMenu::separator {height:1px;background:%5;margin:4px 10px;}
      QSplitter::handle {background:%2;width:14px;}
      QSplitter::handle:hover {background:#DCDDD7;}
      QProgressBar {background:#DFE2E8;border:none;border-radius:3px;max-height:5px;}
      QProgressBar::chunk {background:%6;border-radius:3px;}
      QToolTip {background:%1;color:white;border:none;padding:8px;}
    )").arg(Ink,Background,Surface,Muted,Line,Accent));
}
}
