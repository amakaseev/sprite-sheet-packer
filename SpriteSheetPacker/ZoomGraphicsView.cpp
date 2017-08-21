#include "ZoomGraphicsView.h"

ZoomGraphicsView::ZoomGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    _backgroundBrush = QBrush(QPixmap("://res/patterns_tweed.png"));
}

void ZoomGraphicsView::drawBackground(QPainter *painter, const QRectF &) {
    painter->resetTransform();
    painter->fillRect(visibleRegion().boundingRect(), _backgroundBrush);
}

void ZoomGraphicsView::wheelEvent(QWheelEvent *event) {
#if defined(Q_OS_OSX)
    if (QApplication::keyboardModifiers() == Qt::AltModifier) {
#endif
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

        if (event->delta() > 0) {
            emit zoomed(true);
        } else {
            emit zoomed(false);
        }
#if defined(Q_OS_OSX)
    } else {
        QGraphicsView::wheelEvent(event);
    }
#endif
}
