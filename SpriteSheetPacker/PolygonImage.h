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

    QVector<b2PolygonShape> shapes;

    void add(const Triangles& other) {
        unsigned short idx = verts.size();
        verts += other.verts;
        for (int i=0; i<other.indices.size(); ++i) {
            indices.push_back(other.indices[i] + idx);
        }
        shapes += other.shapes;
    }
};

class PolygonImage
{
public:
    static Triangles generateTriangles(const QImage& image, const QRectF& rect, const float epsilon = 2.f, const float threshold = 0.0f);

protected:
    PolygonImage(const QImage& image, const float epsilon, const float threshold);

    std::vector<QPointF> trace(const QRectF& rect, const float& threshold, Triangles* triangles = NULL);
    QPair<bool, QPointF> findFirstNoneTransparentPixel(const QRectF& rect, const float& threshold, Triangles* triangles = NULL);

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
