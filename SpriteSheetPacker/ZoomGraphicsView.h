#ifndef ZOOMGRAPHICSVIEW_H
#define ZOOMGRAPHICSVIEW_H

#include <QtWidgets>

class ZoomGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ZoomGraphicsView(QWidget *parent = 0);

protected:
    virtual void drawBackground(QPainter *painter, const QRectF &rect);
    virtual void wheelEvent(QWheelEvent* event);

signals:
    void zoomed(bool in);

private:
    QBrush  _backgroundBrush;
};


#endif // ZOOMGRAPHICSVIEW_H
