#ifndef ANIMATIONDIALOG_H
#define ANIMATIONDIALOG_H

#include <QtWidgets>

class SpritesTreeWidget;

namespace Ui {
class AnimationDialog;
}

struct AnimationInfo {
    QString name;
    QList< QPair<QString, QString> > frames;
};

class AnimationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnimationDialog(SpritesTreeWidget* spritesTreeWidget, QWidget *parent = 0);
    ~AnimationDialog();

    static AnimationDialog* instance() { return _instance; }

    void setSpritesTreeModel(QAbstractItemModel* model);
    void setPreviewPixmap(const QPixmap& pixmap);

protected:
    AnimationInfo detectAnimations(const QPair<QString, QString>& item, QList< QPair<QString, QString> >& items);
    void timerEvent(QTimerEvent* event);

private slots:
    void on_framePerSecondSpinBox_valueChanged(int arg1);
    void on_framesSlider_valueChanged(int value);
    void on_playToolButton_toggled(bool checked);
    void on_prevFrameToolButton_clicked();
    void on_nextFrameToolButton_clicked();
    void on_firstFrameToolButton_clicked();
    void on_lastFrameToolButton_clicked();
    void on_autoDetectPushButton_clicked();
    void on_comboBox_currentIndexChanged(int index);

    void on_framesListWidget_currentRowChanged(int currentRow);

private:
    static AnimationDialog*  _instance;

    Ui::AnimationDialog*     ui;
    QGraphicsScene                  _scene;
    QGraphicsPixmapItem*            _pixmapItem;
    QVector<AnimationInfo>          _animations;
    int                             _animationTimer;
    SpritesTreeWidget*              _spritesTreeWidget;
};

#endif // ANIMATIONDIALOG_H
