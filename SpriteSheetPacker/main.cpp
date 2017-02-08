#include "MainWindow.h"
#include "SpritePackerProjectFile.h"
#include <QApplication>

int commandLine(QCoreApplication& app);

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("amakaseev");
    QCoreApplication::setOrganizationDomain("spicyminds-lab.com");
    QCoreApplication::setApplicationName("SpriteSheetPacker");
    QCoreApplication::setApplicationVersion("1.0.8");

    SpritePackerProjectFile::factory().set<SpritePackerProjectFile>("json");
    SpritePackerProjectFile::factory().set<SpritePackerProjectFile>("ssp");
    SpritePackerProjectFile::factory().set<SpritePackerProjectFileTPS>("tps");

    if (argc > 1) {
        return commandLine(app);
    } else {
        MainWindow* wnd = new MainWindow();
        wnd->show();
        return app.exec();
    }
}
