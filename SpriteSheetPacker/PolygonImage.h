#ifndef POLYGONIMAGE_H
#define POLYGONIMAGE_H

#include <QtCore>
#include <QtGui>

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

    std::vector<QPointF> debugPoints;
    std::vector<int> debugPartInfo;

    void add(const Triangles& other) {
        unsigned short idx = verts.size();
        verts += other.verts;
        for (int i=0; i<other.indices.size(); ++i) {
            indices.push_back(other.indices[i] + idx);
        }
        debugPartInfo.push_back(other.indices.size() / 3);
    }
};

typedef std::vector<std::vector<QPointF>> Polygons;

class PolygonImage
{
public:
    PolygonImage(const QImage& image, const QRectF& rect, const float epsilon = 2.f, const float threshold = 0.05f);

    const Triangles& triangles() const { return _triangles; }
    const Polygons& polygons() const { return _polygons; }

protected:
    std::vector<QPointF> trace(const QRectF& rect, const float& threshold);
    QPair<bool, QPointF> findFirstNoneTransparentPixel(const QRectF& rect, const float& threshold);

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
    bool polyInPoly(std::vector<QPointF>& a, const std::vector<QPointF>& b);

    Triangles triangulate(const std::vector<QPointF>& points);
    void calculateUV(const QRectF& rect, V2F_T2F* verts, const size_t& count);

private:
    QImage        _image;
    unsigned int  _width;
    unsigned int  _height;
    unsigned int  _threshold;

    // out
    Triangles     _triangles;
    Polygons      _polygons;
};

#endif // POLYGONIMAGE_H
