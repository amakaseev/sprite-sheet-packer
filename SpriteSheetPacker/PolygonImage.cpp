
#include "PolygonImage.h"
#include "clipper.hpp"
#include "poly2tri.h"

const static float PRECISION = 10.f;

/** Clamp a value between from and to.
 */

inline float clampf(float value, float min_inclusive, float max_inclusive)
{
    if (min_inclusive > max_inclusive) {
        std::swap(min_inclusive, max_inclusive);
    }
    return value < min_inclusive ? min_inclusive : value < max_inclusive? value : max_inclusive;
}

PolygonImage::PolygonImage(const QImage& image, const QRectF& rect, const float epsilon, const float threshold)
    : _width(image.width())
    , _height(image.height())
    , _threshold(threshold)
{
    _image = image.convertToFormat(QImage::Format_RGBA8888);

    QRectF realRect = rect;

    // find first bigger
    double area_big = 0;
    std::vector<QPointF> p_big;
    {
        auto p = trace(realRect, threshold);
        while (p.size() >= 3) {
            // calculate area of polygon
            std::vector<QPointF> polyPoint = p;
            if (polyPoint.size() >= 9) {
                polyPoint = reduce(polyPoint, realRect, epsilon);
            }
            if (polyPoint.size() >= 3) {
                polyPoint = expand(polyPoint, realRect, epsilon);
            }

            // erase contour for find next
            QPolygonF fillPolygon(QVector<QPointF>::fromStdVector(polyPoint));
            fillPolygon.translate(rect.x(), rect.y());
            QColor fillColor(0, 0, 0, 0);
            QPen pen(fillColor);
            pen.setWidthF(2);//qMax(epsilon, 2.f));
            QPainter painer(&_image);
            painer.setPen(pen);
            painer.setBrush(QBrush(fillColor));
            painer.setCompositionMode(QPainter::CompositionMode_Source);
            painer.drawPolygon(fillPolygon);
            painer.end();

            if (polyPoint.size() >= 3) {
                ClipperLib::Path poly;
                for(auto it = polyPoint.begin(); it<polyPoint.end(); ++it) {
                    poly << ClipperLib::IntPoint(it->x()* PRECISION, it->y() * PRECISION);
                }
                double area = fabs(ClipperLib::Area(poly));
                if (area > area_big) {
                    p_big = p;
                    area_big = area;
                }
            }

            // next
            p = trace(realRect, threshold);
        }
    }

    if (p_big.size() < 3) return;

    // reinit image
    _image = image.convertToFormat(QImage::Format_RGBA8888);

    // finding all polygons (start with bigger)
    auto p = p_big;
    while (p.size() >= 3) {
        if (p.size() >= 9) {
            p = reduce(p, realRect, epsilon);
        }
        if (p.size() >= 3) {
            p = expand(p, realRect, epsilon);
        }

        // erase contour for find next
        QPolygonF fillPolygon(QVector<QPointF>::fromStdVector(p));
        fillPolygon.translate(rect.x(), rect.y());
        QColor fillColor(0, 0, 0, 0);
        QPen pen(fillColor);
        pen.setWidthF(2);//qMax(epsilon * 2, 10.f));
        QPainter painer(&_image);
        painer.setPen(pen);
        painer.setBrush(QBrush(fillColor));
        painer.setCompositionMode(QPainter::CompositionMode_Source);
        painer.drawPolygon(fillPolygon);
        painer.end();

        if (p.size() >= 3) {
            _polygons.push_back(p);
        }

        // find next
        p = trace(realRect, threshold);
    }

    // combine all polygons if posible
    bool isCombine = true;
    while (isCombine) {
        isCombine = false;
        for (auto it = _polygons.begin(); it != _polygons.end(); ++it) {
            auto it_test = std::next(it, 1);
            while (it_test != _polygons.end()) {
                if (combine((*it), (*it_test), realRect, epsilon)) {
                    it_test = _polygons.erase(it_test);
                    isCombine = true;
                    continue;
                }
                ++it_test;
            }
        }
    }

    // triangulate polygon(s)
    auto it_p1 = _polygons.begin();
    while (it_p1 != _polygons.end()) {
        auto tri = triangulate((*it_p1));
        if (tri.indices.size()) {
            _triangles.add(tri);
        }
        _triangles.debugPoints.insert(_triangles.debugPoints.end(), (*it_p1).begin(), (*it_p1).end());
        ++it_p1;
    }
}

std::vector<QPointF> PolygonImage::trace(const QRectF& rect, const float& threshold) {
    auto result = findFirstNoneTransparentPixel(rect, threshold);
    if (result.first) {
        return marchSquare(rect, result.second, threshold);
    } else {
        return std::vector<QPointF>();
    }
}

QPair<bool, QPointF> PolygonImage::findFirstNoneTransparentPixel(const QRectF& rect, const float& threshold) {
    QPointF i;
    for(i.ry() = rect.top(); i.y() < rect.bottom(); i.ry()++) {
        for(i.rx() = rect.left(); i.x() < rect.right(); i.rx()++) {
            auto alpha = getAlphaByPos(i);
            if(alpha > threshold) {
                return qMakePair(true, i);
            }
        }
    }
    return qMakePair(false, i);
}

unsigned char PolygonImage::getAlphaByIndex(const unsigned int& i) {
    return getAlphaByPos(getPosFromIndex(i));
}

unsigned char PolygonImage::getAlphaByPos(const QPointF& pos) {
    return qAlpha(_image.pixel(QPoint(pos.x(), pos.y())));
}

unsigned int PolygonImage::getSquareValue(const unsigned int& x, const unsigned int& y, const QRectF& rect, const float& threshold)
{
    /*
     checking the 2x2 pixel grid, assigning these values to each pixel, if not transparent
     +---+---+
     | 1 | 2 |
     +---+---+
     | 4 | 8 | <- current pixel (curx,cury)
     +---+---+
     */
    unsigned int sv = 0;
    //NOTE: due to the way we pick points from texture, rect needs to be smaller, otherwise it goes outside 1 pixel
    auto fixedRect = QRectF(rect.topLeft(), rect.size()-QSize(2,2));

    QPointF tl = QPointF(x-1, y-1);
    sv += (fixedRect.contains(tl) && getAlphaByPos(tl) > threshold)? 1 : 0;
    QPointF tr = QPointF(x, y-1);
    sv += (fixedRect.contains(tr) && getAlphaByPos(tr) > threshold)? 2 : 0;
    QPointF bl = QPointF(x-1, y);
    sv += (fixedRect.contains(bl) && getAlphaByPos(bl) > threshold)? 4 : 0;
    QPointF br = QPointF(x, y);
    sv += (fixedRect.contains(br) && getAlphaByPos(br) > threshold)? 8 : 0;
//    Q_ASSERT_X(sv != 0 && sv != 15, "square value should not be 0, or 15", "");
    return sv;
}

std::vector<QPointF> PolygonImage::marchSquare(const QRectF& rect, const QPointF& start, const float& threshold)
{
    int stepx = 0;
    int stepy = 0;
    int prevx = 0;
    int prevy = 0;
    int startx = start.x();
    int starty = start.y();
    int curx = startx;
    int cury = starty;
    unsigned int count = 0;
    unsigned int totalPixel = _width*_height;
    bool problem = false;
    std::vector<int> case9s;
    std::vector<int> case6s;
    int i;
    std::vector<int>::iterator it;
    std::vector<QPointF> _points;
    do{
        int sv = getSquareValue(curx, cury, rect, threshold);
        switch(sv) {
            case 1:
            case 5:
            case 13:
                /* going UP with these cases:
                 1          5           13
                 +---+---+  +---+---+  +---+---+
                 | 1 |   |  | 1 |   |  | 1 |   |
                 +---+---+  +---+---+  +---+---+
                 |   |   |  | 4 |   |  | 4 | 8 |
                 +---+---+  +---+---+  +---+---+
                 */
                stepx = 0;
                stepy = -1;
                break;
            case 8:
            case 10:
            case 11:
                /* going DOWN with these cases:
                 8          10          11
                 +---+---+  +---+---+   +---+---+
                 |   |   |  |   | 2 |   | 1 | 2 |
                 +---+---+  +---+---+   +---+---+
                 |   | 8 |  |   | 8 |   |   | 8 |
                 +---+---+  +---+---+  	+---+---+
                 */
                stepx = 0;
                stepy = 1;
                break;
            case 4:
            case 12:
            case 14:
                /* going LEFT with these cases:
                 4          12          14
                 +---+---+  +---+---+   +---+---+
                 |   |   |  |   |   |   |   | 2 |
                 +---+---+  +---+---+   +---+---+
                 | 4 |   |  | 4 | 8 |   | 4 | 8 |
                 +---+---+  +---+---+  	+---+---+
                 */
                stepx = -1;
                stepy = 0;
                break;
            case 2 :
            case 3 :
            case 7 :
                /* going RIGHT with these cases:
                 2          3           7
                 +---+---+  +---+---+   +---+---+
                 |   | 2 |  | 1 | 2 |   | 1 | 2 |
                 +---+---+  +---+---+   +---+---+
                 |   |   |  |   |   |   | 4 |   |
                 +---+---+  +---+---+  	+---+---+
                 */
                stepx=1;
                stepy=0;
                break;
            case 9 :
                /*
                 +---+---+
                 | 1 |   |
                 +---+---+
                 |   | 8 |
                 +---+---+
                 this should normally go UP, but if we already been here, we go down
                */
                //find index from xy;
                i = getIndexFromPos(curx, cury);
                it = find (case9s.begin(), case9s.end(), i);
                if (it != case9s.end())
                {
                    //found, so we go down, and delete from case9s;
                    stepx = 0;
                    stepy = 1;
                    case9s.erase(it);
                    problem = true;
                }
                else
                {
                    //not found, we go up, and add to case9s;
                    stepx = 0;
                    stepy = -1;
                    case9s.push_back(i);
                }
                break;
            case 6 :
                /*
                 6
                 +---+---+
                 |   | 2 |
                 +---+---+
                 | 4 |   |
                 +---+---+
                 this normally go RIGHT, but if its coming from UP, it should go LEFT
                 */
                i = getIndexFromPos(curx, cury);
                it = find (case6s.begin(), case6s.end(), i);
                if (it != case6s.end())
                {
                    //found, so we go down, and delete from case9s;
                    stepx = -1;
                    stepy = 0;
                    case6s.erase(it);
                    problem = true;
                }
                else{
                    //not found, we go up, and add to case9s;
                    stepx = 1;
                    stepy = 0;
                    case6s.push_back(i);
                }
                break;
            default:
                qDebug() << "this shouldn't happen:" << _points.size();
                return _points;
        }
        //little optimization
        // if previous direction is same as current direction,
        // then we should modify the last vec to current
        curx += stepx;
        cury += stepy;
//        _points.push_back(QPointF(curx - rect.left(), rect.size().height() - cury + rect.top()));
        if(stepx == prevx && stepy == prevy && _points.size()) {
            _points.back().setX(curx - rect.left());
            _points.back().setY(cury - rect.top());
        } else if(problem) {
            //TODO: we triangulation cannot work collinear points, so we need to modify same point a little
            //TODO: maybe we can detect if we go into a hole and coming back the hole, we should extract those points and remove them
            _points.push_back(QPointF(curx - rect.left(), cury - rect.top()));
        } else {
            _points.push_back(QPointF(curx - rect.left(), cury - rect.top()));
        }

        count++;
        prevx = stepx;
        prevy = stepy;
        problem = false;
        Q_ASSERT_X(count <= totalPixel, "oh no, marching square cannot find starting position", "");
    } while(curx != startx || cury != starty);
    return _points;
}

float PolygonImage::perpendicularDistance(const QPointF& i, const QPointF& start, const QPointF& end) {
    float res;
    float slope;
    float intercept;

    if(start.x() == end.x()) {
        res = abs(i.x() - end.x());
    } else if (start.y() == end.y()) {
        res = abs(i.y() - end.y());
    } else {
        slope = (end.y() - start.y()) / (end.x() - start.x());
        intercept = start.y() - (slope * start.x());
        res = fabsf(slope * i.x() - i.y() + intercept) / sqrtf(powf(slope, 2) + 1);
    }
    return res;
}

std::vector<QPointF> PolygonImage::rdp(std::vector<QPointF> v, const float& optimization) {
    if(v.size() < 3)
        return v;

    int index = -1;
    float dist = 0;
    //not looping first and last point
    for(size_t i = 1; i < v.size()-1; i++)
    {
        float cdist = perpendicularDistance(v[i], v.front(), v.back());
        if(cdist > dist)
        {
            dist = cdist;
            index = i;
        }
    }
    if (dist>optimization)
    {
        std::vector<QPointF>::const_iterator begin = v.begin();
        std::vector<QPointF>::const_iterator end   = v.end();
        std::vector<QPointF> l1(begin, begin+index+1);
        std::vector<QPointF> l2(begin+index, end);

        std::vector<QPointF> r1 = rdp(l1, optimization);
        std::vector<QPointF> r2 = rdp(l2, optimization);

        r1.insert(r1.end(), r2.begin()+1, r2.end());
        return r1;
    }
    else {
        std::vector<QPointF> ret;
        ret.push_back(v.front());
        ret.push_back(v.back());
        return ret;
    }
}

std::vector<QPointF> PolygonImage::reduce(const std::vector<QPointF>& points, const QRectF& rect , const float& epsilon) {
    //return points;
    auto size = points.size();
    // if there are less than 3 points, then we have nothing
    if (size < 3) {
        qDebug("AUTOPOLYGON: cannot reduce points that has less than 3 points in input, e: %f", epsilon);
        return std::vector<QPointF>();
    }
    // if there are less than 9 points (but more than 3), then we don't need to reduce it
    else if (size < 9) {
        qDebug("AUTOPOLYGON: cannot reduce points epsilon: %f", epsilon);
        return points;
    }

    float maxEp = qMin(rect.size().width(), rect.size().height());
    float ep = clampf(epsilon, 0.0f, maxEp / 2);
    std::vector<QPointF> result = rdp(points, ep);

    auto last = result.back();

    QPointF pointDistance = last - result.front();

    if (last.y() > result.front().y() && pointDistance.manhattanLength() < ep * 0.5f) {
        result.front().setY(last.y());
        result.pop_back();
    }
    return result;
}

std::vector<QPointF> PolygonImage::expand(const std::vector<QPointF>& points, const QRectF &rect, const float& epsilon) {
    // if there are less than 3 points, then we have nothing
    if(points.size() < 3) {
        qDebug("AUTOPOLYGON: cannot expand points with less than 3 points, e: %f", epsilon);
        return std::vector<QPointF>();
    }
    ClipperLib::Path subj;
    ClipperLib::PolyTree solution;
    ClipperLib::PolyTree out;
    for(std::vector<QPointF>::const_iterator it = points.begin(); it<points.end(); it++) {
        subj << ClipperLib::IntPoint(it->x()* PRECISION, it->y() * PRECISION);
    }

    // clean
//    ClipperLib::CleanPolygon(subj, PRECISION * 2);
//    if (subj.size() < 3) {
//        return std::vector<QPointF>();
//    }

    ClipperLib::ClipperOffset co;
    co.AddPath(subj, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    co.Execute(solution, epsilon * PRECISION);

    ClipperLib::PolyNode* p = solution.GetFirst();
    if(!p) {
        qDebug("AUTOPOLYGON: Clipper failed to expand the points");
        return points;
    }
    while(p->IsHole()){
        p = p->GetNext();
    }

    //turn the result into simply polygon (AKA, fix overlap)

    //clamp into the specified rect
    ClipperLib::Clipper cl;
    cl.StrictlySimple(true);
    cl.AddPath(p->Contour, ClipperLib::ptSubject, true);
    //create the clipping rect
    ClipperLib::Path clamp;
    clamp.push_back(ClipperLib::IntPoint(0, 0));
    clamp.push_back(ClipperLib::IntPoint(rect.size().width() * PRECISION, 0));
    clamp.push_back(ClipperLib::IntPoint(rect.size().width() * PRECISION, rect.size().height() * PRECISION));
    clamp.push_back(ClipperLib::IntPoint(0, rect.size().height() * PRECISION));
    cl.AddPath(clamp, ClipperLib::ptClip, true);
    cl.Execute(ClipperLib::ctIntersection, out);

    std::vector<QPointF> outPoints;
    ClipperLib::PolyNode* p2 = out.GetFirst();
    while(p2->IsHole()){
        p2 = p2->GetNext();
    }
    auto end = p2->Contour.end();
    for(std::vector<ClipperLib::IntPoint>::const_iterator pt = p2->Contour.begin(); pt < end; pt++)
    {
        outPoints.push_back(QPointF(pt->X/PRECISION, pt->Y/PRECISION));
    }
    return outPoints;
}

bool polyInPoly(const ClipperLib::Path& subjA, const ClipperLib::Path& subjB) {
    for (auto p: subjA) {
        if (ClipperLib::PointInPolygon(p, subjB) == 1) {
            return true;
        }
    }
    for (auto p: subjB) {
        if (ClipperLib::PointInPolygon(p, subjA) == 1) {
            return true;
        }
    }

    return false;
}

bool PolygonImage::combine(std::vector<QPointF>& a, const std::vector<QPointF>& b, const QRectF &rect, const float& epsilon) {
    ClipperLib::Path subjA;
    for(std::vector<QPointF>::const_iterator it = a.begin(); it<a.end(); it++) {
        subjA << ClipperLib::IntPoint(it->x() * PRECISION, it->y() * PRECISION);
    }

    ClipperLib::Path subjB;
    for(std::vector<QPointF>::const_iterator it = b.begin(); it<b.end(); it++) {
        subjB << ClipperLib::IntPoint(it->x() * PRECISION, it->y() * PRECISION);
    }

    if (!polyInPoly(subjA, subjB)) return false;

//    ClipperLib::PolyTree solution;
    ClipperLib::PolyTree out;

    //clamp into the specified rect
    ClipperLib::Clipper cl;
    cl.StrictlySimple(true);
    cl.AddPath(subjA, ClipperLib::ptSubject, true);
    cl.AddPath(subjB, ClipperLib::ptClip, true);
    cl.Execute(ClipperLib::ctUnion, out);

    ClipperLib::PolyNode* p2 = out.GetFirst();
    while(p2->IsHole()){
        p2 = p2->GetNext();
    }
    auto end = p2->Contour.end();

    a.clear();
    for(std::vector<ClipperLib::IntPoint>::const_iterator pt = p2->Contour.begin(); pt < end; pt++) {
        a.push_back(QPointF(pt->X/PRECISION, pt->Y/PRECISION));
    }

    return true;
}

Triangles PolygonImage::triangulate(const std::vector<QPointF>& points) {
    // if there are less than 3 points, then we can't triangulate
    if(points.size()<3)
    {
        qDebug("AUTOPOLYGON: cannot triangulate with less than 3 points");
        return Triangles();
    }
    std::vector<p2t::Point*> p2points;
    for(std::vector<QPointF>::const_iterator it = points.begin(); it<points.end(); it++)
    {
        p2t::Point * p = new p2t::Point(it->x(), it->y());
        p2points.push_back(p);
    }

    p2t::CDT cdt(p2points);
    cdt.Triangulate();
    std::vector<p2t::Triangle*> tris = cdt.GetTriangles();

    Triangles triangles;
    unsigned short idx = 0;
    unsigned short vdx = 0;

    for(std::vector<p2t::Triangle*>::const_iterator ite = tris.begin(); ite < tris.end(); ite++) {
        for(int i = 0; i < 3; i++) {
            auto p = (*ite)->GetPoint(i);
            auto v2 = QPointF(p->x, p->y);
            bool found = false;
            size_t j;
            size_t length = vdx;
            for(j = 0; j < length; j++) {
                if(triangles.verts[j].v == v2) {
                    found = true;
                    break;
                }
            }
            if(found) {
                //if we found the same vertex, don't add to verts, but use the same vertex with indices
                triangles.indices.push_back(j);
                idx++;
            } else {
                //vert does not exist yet, so we need to create a new one,
                auto t2f = QPointF(0,0); // don't worry about tex coords now, we calculate that later
                V2F_T2F vert = {v2, t2f};
                triangles.verts.push_back(vert);
                triangles.indices.push_back(vdx);
                idx++;
                vdx++;
            }
        }
    }
    for(auto j : p2points)
    {
        delete j;
    };
    return triangles;
}

void PolygonImage::calculateUV(const QRectF& rect, V2F_T2F* verts, const size_t& count) {
    /*
     whole texture UV coordination
     0,0                  1,0
     +---------------------+
     |                     |0.1
     |                     |0.2
     |     +--------+      |0.3
     |     |texRect |      |0.4
     |     |        |      |0.5
     |     |        |      |0.6
     |     +--------+      |0.7
     |                     |0.8
     |                     |0.9
     +---------------------+
     0,1                  1,1
     */

    Q_ASSERT_X(_width && _height, "please specify width and height for this AutoPolygon instance", "");
    float texWidth  = _width;
    float texHeight = _height;

    auto end = &verts[count];
    for(auto i = verts; i != end; i++) {
        // for every point, offset with the center point
        float u = (i->v.x() + rect.left()) / texWidth;
        float v = (rect.top()+rect.size().height() - i->v.y()) / texHeight;
        i->t.setX(u);
        i->t.setY(v);
    }
}
