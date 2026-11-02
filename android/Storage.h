#pragma once
#include <QByteArray>
#include <QImage>
#include <QString>
namespace storage {
QByteArray read(const QString& path,QString& error);
bool savePng(const QString& path,const QImage& image);
}
