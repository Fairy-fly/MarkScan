#include "desktop/ImageCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTouchEvent>
#include <cmath>
#include <algorithm>
ImageCanvas::ImageCanvas(bool interactive,QWidget* parent):QWidget(parent),interactive_(interactive) {
    setMinimumSize(100,160);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    if(interactive_)setAttribute(Qt::WA_AcceptTouchEvents);setAccessibleName("答题卡图像预览");
}
void ImageCanvas::setImage(const QImage& image){image_=image;fit();}
void ImageCanvas::fit(){fitted_=true;offset_={};if(!image_.isNull())scale_=std::min((width()-24.)/image_.width(),(height()-24.)/image_.height());if(scaleChanged)scaleChanged(scale_);update();}
void ImageCanvas::actualSize(){fitted_=false;scale_=1;offset_={};if(scaleChanged)scaleChanged(scale_);update();}
void ImageCanvas::zoomBy(double factor){zoomAt(factor,rect().center());}
void ImageCanvas::zoomAt(double factor,QPointF anchor) {
    if(image_.isNull())return;fitted_=false;double next=std::clamp(scale_*factor,0.04,8.);double ratio=next/scale_;
    QPointF center=QPointF(width()/2.,height()/2.);offset_=(offset_+center-anchor)*ratio+anchor-center;scale_=next;
    if(scaleChanged)scaleChanged(scale_);update();
}
void ImageCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);p.setBrush(QColor("#E7E8E3"));p.drawRoundedRect(rect(),10,10);
    if(image_.isNull())return;p.setRenderHint(QPainter::SmoothPixmapTransform);p.setClipRect(rect());
    QSizeF size=image_.size()*scale_;QRectF target(QPointF((width()-size.width())/2.,(height()-size.height())/2.)+offset_,size);
    p.drawImage(target,image_);
}
void ImageCanvas::resizeEvent(QResizeEvent*){if(fitted_)fit();}
void ImageCanvas::wheelEvent(QWheelEvent* e){if(interactive_){zoomAt(std::pow(1.0015,e->angleDelta().y()),e->position());e->accept();}else e->ignore();}
void ImageCanvas::mousePressEvent(QMouseEvent* e){pressed_=last_=e->position();dragged_=false;if(interactive_)setCursor(Qt::ClosedHandCursor);}
void ImageCanvas::mouseMoveEvent(QMouseEvent* e){if((e->buttons()&Qt::LeftButton)&&interactive_){offset_+=e->position()-last_;last_=e->position();fitted_=false;dragged_=true;update();}}
void ImageCanvas::mouseReleaseEvent(QMouseEvent* e){unsetCursor();if(!dragged_&&QLineF(pressed_,e->position()).length()<8&&activated)activated();}
bool ImageCanvas::event(QEvent* e) {
    if(interactive_&&(e->type()==QEvent::TouchBegin||e->type()==QEvent::TouchUpdate||e->type()==QEvent::TouchEnd||e->type()==QEvent::TouchCancel)) {
        auto t=static_cast<QTouchEvent*>(e);auto pts=t->points();
        if(e->type()==QEvent::TouchEnd||e->type()==QEvent::TouchCancel){touchDistance_=0;e->accept();return true;}
        if(!pts.isEmpty()) {
            QPointF c=pts[0].position();double distance=0;
            if(pts.size()>1){c=(pts[0].position()+pts[1].position())/2.;distance=QLineF(pts[0].position(),pts[1].position()).length();}
            if(e->type()==QEvent::TouchUpdate){offset_+=c-touchCenter_;fitted_=false;if(distance>0&&touchDistance_>0)zoomAt(distance/touchDistance_,c);update();}
            touchCenter_=c;touchDistance_=distance;
        }
        e->accept();return true;
    }
    return QWidget::event(e);
}
