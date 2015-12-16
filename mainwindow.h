#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "spriteatlas.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void refreshOpenRecentMenu();

    void refreshAtlas();
    void refreshSpritesTree(const QStringList& fileList);
    void recursiveRefreshFolder(const QString& folder, QTreeWidgetItem* parentItem);
    QStringList fileListFromTree();

    void openSpritePack(const QString& fileName);
    void saveSpritePack(const QString& fileName);
    void publishSpriteSheet(const QString& fileName, const QString& texName, const QMap<QString, SpriteFrameInfo>& spriteFrames);

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void openRecent();

    void on_actionRefresh_triggered();

    // zoom control
    void on_toolButtonZoomOut_clicked();
    void on_toolButtonZoomIn_clicked();
    void on_toolButtonZoom1x1_clicked();
    void on_toolButtonZoomFit_clicked();
    void on_zoomSlider_valueChanged(int value);

    // source sprite control
    void on_spritesTreeWidget_itemSelectionChanged();
    void on_toolButtonAddSprites_clicked();
    void on_toolButtonAddFolder_clicked();
    void on_toolButtonRemoveSprites_clicked();

    void on_output_destFolderToolButton_clicked();
    void on_output_publishPushButton_clicked();

private:
    Ui::MainWindow*      ui;
    QGraphicsScene*     _scene;
};

#endif // MAINWINDOW_H
