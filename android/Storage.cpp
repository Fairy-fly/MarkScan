#include "android/Storage.h"
#include <QFile>
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#include <QJniEnvironment>
#include <QDebug>
namespace {
int contentFd(const QString& path,const char* mode) {
    // Open the returned URI directly. Qt 6.5's cached URI file engine can
    // lose its lookup on this Android 16 system picker; never resolve a
    // content URI to a guessed filesystem path or request broad storage access.
    QJniEnvironment env;
    QJniObject context(QNativeInterface::QAndroidApplication::context());
    auto resolver=context.callObjectMethod("getContentResolver","()Landroid/content/ContentResolver;");
    auto uri=QJniObject::callStaticObjectMethod("android/net/Uri","parse","(Ljava/lang/String;)Landroid/net/Uri;",QJniObject::fromString(path).object<jstring>());
    auto descriptor=resolver.callObjectMethod("openFileDescriptor","(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",uri.object(),QJniObject::fromString(QString::fromLatin1(mode)).object<jstring>());
    if(env->ExceptionCheck()) {env->ExceptionDescribe();env->ExceptionClear();return -1;}
    if(!descriptor.isValid())return -1;
    int fd=descriptor.callMethod<jint>("detachFd","()I");
    descriptor.callMethod<void>("close","()V");
    return fd;
}
}
#endif
namespace storage {
QByteArray read(const QString& path,QString& error) {
    QFile file;
#ifdef Q_OS_ANDROID
    if(path.startsWith("content://")) {
        int fd=contentFd(path,"r");
        if(fd<0 || !file.open(fd,QIODevice::ReadOnly,QFileDevice::AutoCloseHandle)) {error=QStringLiteral("无法读取所选图片 URI");return {};}
    } else
#endif
    {file.setFileName(path);if(!file.open(QIODevice::ReadOnly)){error=file.errorString();return {};}}
    constexpr qint64 limit=50*1024*1024;
    auto data=file.read(limit+1);
    if(data.size()>limit){error=QStringLiteral("文件超过 50 MB，请缩小图片后导入");return {};}
    if(data.isEmpty())error=QStringLiteral("文件为空或读取失败");
    return data;
}
bool savePng(const QString& path,const QImage& image) {
    QFile file;
#ifdef Q_OS_ANDROID
    if(path.startsWith("content://")) {
        int fd=contentFd(path,"w");
        if(fd<0 || !file.open(fd,QIODevice::WriteOnly,QFileDevice::AutoCloseHandle))return false;
    } else
#endif
    {file.setFileName(path);if(!file.open(QIODevice::WriteOnly))return false;}
    return image.save(&file,"PNG") && file.flush();
}
}
