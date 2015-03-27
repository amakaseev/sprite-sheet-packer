#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>

namespace Ui {
class MainWindow;
}

class SpriteFrameInfo {
public:
    QRect   mFrame;
    QPoint  mOffset;
    bool    mRotated;
    QRect   mSourceColorRect;
    QSize   mSourceSize;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void refreshOpenRecentMenu();
    void generateAtlas(float scale, QImage& atlasImage, QMap<QString, SpriteFrameInfo>& spriteFrames);
    void refreshAtlas();
    void refreshSpritesTree(const QStringList& fileList);
    void recursiveRefreshFolder(const QString& folder, QTreeWidgetItem* parentItem);
    void openSpritePack(const QString& fileName);
    void saveSpritePack(const QString& fileName);
    void publishSpriteSheet(const QString& fileName, const QString& texName, QMap<QString, SpriteFrameInfo>& spriteFrames);

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void openRecent();

    void on_actionAddSprites_triggered();
    void on_actionAddFolder_triggered();
    void on_actionRemove_triggered();
    void on_actionRefresh_triggered();
    void on_actionZoomIn_triggered();
    void on_actionZoomOut_triggered();

    void on_output_destFolderToolButton_clicked();
    void on_output_publishPushButton_clicked();

private:
    Ui::MainWindow*      ui;
    QGraphicsScene*     _scene;
    QGraphicsView*      _graphicsView;
};

#endif // MAINWINDOW_H
