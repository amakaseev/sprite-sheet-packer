#ifndef SPRITESTREEWIDGET_H
#define SPRITESTREEWIDGET_H

#include <QtWidgets>

class SpritesTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    SpritesTreeWidget(QWidget* parent = NULL);

    QStringList contentList();
    QList< QPair<QString, QString> > fileList();

    void addContent(const QStringList& content, QTreeWidgetItem* parentItem = NULL);
    void refresh();
};

#endif // SPRITESTREEWIDGET_H
