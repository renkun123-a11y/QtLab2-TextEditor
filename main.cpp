#include "appinfo.h"
#include "mainwindow.h"

#include <QApplication>
#include <QIcon>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QString::fromUtf8(APP_NAME));
    QApplication::setApplicationVersion(QString::fromUtf8(APP_VERSION_FULL));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    MainWindow window;
    window.show();

    // 支持“用程序打开某个文件”：在命令行里传入路径即可。
    if (argc > 1) {
        const QString path = QString::fromLocal8Bit(argv[1]);
        QTimer::singleShot(0, &window, [&window, path] { window.openPath(path); });
    }

    return app.exec();
}
