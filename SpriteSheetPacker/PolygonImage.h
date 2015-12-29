#ifndef POLYGONIMAGE_H
#define POLYGONIMAGE_H

#include <QtCore>
#include <QImage>

#include <Box2D.h>

struct V2F_T2F {
    QPointF v;
    QPointF t;
};

struct Triangles {
    /**Vertex data pointer.*/
    QVector<V2F_T2F> verts;
    /**Index data pointer.*/
    QVector<unsigned short> indices;
};

class PolygonImage
{
public:
    static Triangles generateTriangles(const QImage& image, const QRectF& rect, const float epsilon = 2.f, const float threshold = 0.05f);

protected:
    PolygonImage(const QImage& image, const float epsilon, const float threshold);

    std::vector<QPointF> traceAll(const QRectF& rect, const float& threshold);
    std::vector<QPointF> trace(const QRectF& rect, const float& threshold);
    QPointF findFirstNoneTransparentPixel(const QRectF& rect, const float& threshold);

    unsigned char getAlphaByIndex(const unsigned int& i);
    unsigned char getAlphaByPos(const QPointF& pos);
    int getIndexFromPos(const unsigned int& x, const unsigned int& y) { return y*_width+x; }
    QPointF getPosFromIndex(const unsigned int& i) { return QPointF(i % _width, i / _width); }

    unsigned int getSquareValue(const unsigned int& x, const unsigned int& y, const QRectF& rect, const float& threshold);
    std::vector<QPointF> marchSquare(const QRectF& rect, const QPointF& start, const float& threshold);
    float perpendicularDistance(const QPointF& i, const QPointF& start, const QPointF& end);
    std::vector<QPointF> rdp(std::vector<QPointF> v, const float& optimization);
    std::vector<QPointF> reduce(const std::vector<QPointF>& points, const QRectF& rect , const float& epsilon);
    std::vector<QPointF> expand(const std::vector<QPointF>& points, const QRectF &rect, const float& epsilon);

    Triangles triangulate(const std::vector<QPointF>& points);
    void calculateUV(const QRectF& rect, V2F_T2F* verts, const ssize_t& count);

private:
    const QImage& _image;
    std::string   _filename;
    unsigned int  _width;
    unsigned int  _height;
    unsigned int  _threshold;
};

#endif // POLYGONIMAGE_H
