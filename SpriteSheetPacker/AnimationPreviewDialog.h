#ifndef ANIMATIONPREVIEWDIALOG_H
#define ANIMATIONPREVIEWDIALOG_H

#include <QtWidgets>

class SpritesTreeWidget;

namespace Ui {
class AnimationPreviewDialog;
}

struct AnimationInfo {
    QString name;
    QList< QPair<QString, QString> > frames;
};

class AnimationPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnimationPreviewDialog(SpritesTreeWidget* spritesTreeWidget, QWidget *parent = 0);
    ~AnimationPreviewDialog();

    static AnimationPreviewDialog* instance() { return _instance; }

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

private:
    static AnimationPreviewDialog*  _instance;

    Ui::AnimationPreviewDialog*     ui;
    QGraphicsScene                  _scene;
    QGraphicsPixmapItem*            _pixmapItem;
    QVector<QPixmap>                _frames;
    int                             _animationTimer;
    SpritesTreeWidget*              _spritesTreeWidget;
};

#endif // ANIMATIONPREVIEWDIALOG_H
