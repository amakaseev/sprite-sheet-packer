#include "MainWindow.h"
#include <QApplication>

int commandLine(QCoreApplication& app);

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
#ifdef Q_OS_WIN32
        QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

    QCoreApplication::setOrganizationName("amakaseev");
    QCoreApplication::setOrganizationDomain("spicyminds-lab.com");
    QCoreApplication::setApplicationName("SpriteSheetPacker");
    QCoreApplication::setApplicationVersion("1.0.2");
    //QDir::setCurrent(QApplication::applicationDirPath());

    if (argc > 1) {
        return commandLine(app);
    } else {
        MainWindow w;
        w.show();
        return app.exec();
    }
}
