#include <QApplication>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QImage>
#include <QDir>
int main(int argc,char**argv) {
    QApplication app(argc,argv);QString root=argc>1?argv[1]:QDir::currentPath();
    QImage sheet(root+"/templates/blank.png");if(sheet.isNull())return 1;
    QPdfWriter pdf(root+"/templates/MarkScan-A4.pdf");pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageMargins(QMarginsF(0,0,0,0));pdf.setResolution(150);pdf.setTitle("MarkScan v1 - A4 Answer Sheet");
    QPainter painter(&pdf);if(!painter.isActive())return 2;painter.drawImage(QRect(0,0,pdf.width(),pdf.height()),sheet);painter.end();return 0;
}
