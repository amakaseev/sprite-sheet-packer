#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Home");
    QCoreApplication::setOrganizationDomain("Home.com");
    QCoreApplication::setApplicationName("SpritePacker");
    QDir::setCurrent(QApplication::applicationDirPath());

    MainWindow w;
    w.show();

    return a.exec();
}
