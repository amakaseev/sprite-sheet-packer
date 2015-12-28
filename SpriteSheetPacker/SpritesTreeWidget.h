#ifndef SPRITESTREEWIDGET_H
#define SPRITESTREEWIDGET_H

#include <QtWidgets>

class SpritesTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    SpritesTreeWidget(QWidget* parent = NULL);

    QStringList contentList();
    void addContent(const QStringList& content, QTreeWidgetItem* parentItem = NULL);
    void refresh();

signals:
    void dropComplete();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dropEvent(QDropEvent* event);
};

#endif // SPRITESTREEWIDGET_H
