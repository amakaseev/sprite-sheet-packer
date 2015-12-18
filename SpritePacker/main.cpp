#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef Q_OS_WIN32
        QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

    QCoreApplication::setOrganizationName("spicyminds-lab");
    QCoreApplication::setOrganizationDomain("spicyminds-lab.com");
    QCoreApplication::setApplicationName("SpritePacker");
    QDir::setCurrent(QApplication::applicationDirPath());

    MainWindow w;
    w.show();

    return a.exec();
}
