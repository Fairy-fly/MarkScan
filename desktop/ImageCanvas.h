#pragma once
#include <QWidget>
#include <QImage>
#include <functional>
class ImageCanvas:public QWidget {
public:
    explicit ImageCanvas(bool interactive,QWidget* parent=nullptr);
    void setImage(const QImage& image);
    void fit();void actualSize();void zoomBy(double factor);
    double scale() const{return scale_;}
    std::function<void()> activated;
    std::function<void(double)> scaleChanged;
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    bool event(QEvent*) override;
private:
    void zoomAt(double factor,QPointF anchor);
    QImage image_;bool interactive_,fitted_=true,dragged_=false;
    double scale_=1,touchDistance_=0;QPointF offset_,last_,pressed_,touchCenter_;
};
