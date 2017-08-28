#ifndef ANIMATIONDIALOG_H
#define ANIMATIONDIALOG_H

#include <QtWidgets>
#include "ElapsedTimer.h"

class SpritesTreeWidget;

namespace Ui {
class AnimationDialog;
}

struct AnimationInfo {
    QString name;
    int fps;
    bool loop;
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

private slots:
    void updateFrame(int elapsed);

    void on_framePerSecondSpinBox_valueChanged(int arg1);
    void on_framesSlider_valueChanged(int value);
    void on_repeatToolButton_clicked(bool checked);
    void on_playToolButton_toggled(bool checked);
    void on_prevFrameToolButton_clicked();
    void on_nextFrameToolButton_clicked();
    void on_firstFrameToolButton_clicked();
    void on_lastFrameToolButton_clicked();
    void on_autoDetectPushButton_clicked();
    void on_animationsComboBox_currentIndexChanged(int index);
    void on_animationsComboBox_editTextChanged(const QString &arg1);
    void on_addAnimationToolButton_clicked();
    void on_removeAnimationToolButton_clicked();
    void on_framesListWidget_currentRowChanged(int currentRow);

private:
    static AnimationDialog*  _instance;

    Ui::AnimationDialog*            ui;
    QGraphicsScene                  _scene;
    QGraphicsPixmapItem*            _pixmapItem;
    QVector<AnimationInfo>          _animations;
    ElapsedTimer*                   _animationTimer;
    float                           _currentFrame;
    SpritesTreeWidget*              _spritesTreeWidget;
};

#endif // ANIMATIONDIALOG_H
