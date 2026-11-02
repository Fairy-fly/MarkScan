#include "desktop/MainWindow.h"
#include "desktop/Theme.h"
#include <QApplication>
#include <QDir>
#include <QTimer>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif
int main(int argc,char**argv) {
    QApplication app(argc,argv);QApplication::setApplicationName("MarkScan");QApplication::setOrganizationName("MarkScan");
    auto args=app.arguments();
#ifdef Q_OS_ANDROID
    const bool mobile=true;
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]{
        QJniObject activity(QNativeInterface::QAndroidApplication::context());
        auto androidWindow=activity.callObjectMethod("getWindow","()Landroid/view/Window;");
        androidWindow.callMethod<void>("addFlags","(I)V",jint(128));
        androidWindow.callMethod<void>("setStatusBarColor","(I)V",jint(0xfff3f2ee));
        androidWindow.callMethod<void>("setNavigationBarColor","(I)V",jint(0xfff3f2ee));
        auto decor=androidWindow.callObjectMethod("getDecorView","()Landroid/view/View;");
        decor.callMethod<void>("setSystemUiVisibility","(I)V",jint(0x2010));
    });
#else
    const bool mobile=args.contains("--mobile-preview");
#endif
    theme::apply(app,mobile);MainWindow window(mobile);if(args.contains("--narrow"))window.resize(320,740);window.show();
    if(args.contains("--smoke")){int i=args.indexOf("--smoke");QTimer::singleShot(300,&window,[&,i]{window.runSmoke(args.value(i+1,QDir::currentPath()));});}
    else if(args.contains("--demo"))window.runDemo();
    else if(args.size()>1&&!args[1].startsWith("--"))QTimer::singleShot(200,&window,[&]{window.importImage(args[1]);});
    return app.exec();
}
