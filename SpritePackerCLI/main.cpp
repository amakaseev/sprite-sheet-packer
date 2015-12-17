#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineParser>

void printHelp() {
    qInfo() <<  "bubububu";
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printHelp();

    return 1;
}

