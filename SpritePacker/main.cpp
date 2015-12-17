#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QCoreApplication::setOrganizationName("spicyminds-lab");
    QCoreApplication::setOrganizationDomain("spicyminds-lab.com");
    QCoreApplication::setApplicationName("SpritePacker");
    QDir::setCurrent(QApplication::applicationDirPath());

    MainWindow w;
    w.show();

    return a.exec();
}
