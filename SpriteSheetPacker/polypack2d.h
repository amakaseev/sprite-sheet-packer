/*
 Copyright (c) 2016, amakaseev < aleksey.makaseev.@gmail.com >
 All rights reserved.
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef POLYPACK2D_H
#define POLYPACK2D_H

#include <QPointF>
#include <QDebug>
#include <math.h>

namespace PolyPack2D {

    // TODO: move to math
    struct Point {
        float x, y;

        Point(): x(0), y(0) { }
        Point(float _x, float _y): x(_x), y(_y) { }

        Point operator + (const Point& p) const {
           return Point(this->x + p.x, this->y + p.y);
        }

        Point operator - (const Point& p) const {
           return Point(this->x - p.x, this->y - p.y);
        }
    };

    struct Rect {
        float left;
        float top;
        float right;
        float bottom;
    };

    std::vector<Point> createConvexHull(std::vector<Point> p);
    float convexHullArea(const std::vector<Point>& p);
    bool convexHullIntersect(const std::vector<Point>& aConvexHull, const std::vector<Point>& bConvexHull);
    /////


    template<class T> class Content {
    public:
        Content(const T &content, std::vector<std::vector<QPointF>> polygons)
        : _content(content)
        , _area(0)
        {
            _offset.x = _offset.y = 0;

            // convert and calculate bounding box
            bool first = true;
            std::vector<Point> allPoints;
            for (auto it = polygons.begin(); it != polygons.end(); ++it) {
                std::vector<Point> polygon;
                for (auto it_p = (*it).begin(); it_p != (*it).end(); ++it_p) {
                    Point point(it_p->x(), it_p->y());
                    polygon.push_back(point);

                    if (first) {
                        _bounds.left = _bounds.right = point.x;
                        _bounds.top = _bounds.bottom = point.y;
                        first = false;
                    } else {
                        if (_bounds.left > point.x) _bounds.left = point.x;
                        if (_bounds.right < point.x) _bounds.right = point.x;
                        if (_bounds.top > point.y) _bounds.top = point.y;
                        if (_bounds.bottom < point.y) _bounds.bottom = point.y;
                    }
                }
                _polygons.push_back(polygon);
                allPoints.insert(allPoints.end(), polygon.begin(), polygon.end());
            }

            _convexHull = createConvexHull(allPoints);
            _area = convexHullArea(_convexHull);
        }
        Content(const Content& other)
            : _content(other._content)
            , _offset(other.offset())
            , _polygons(other._polygons)
            , _convexHull(other._convexHull)
            , _area(other._area)
            , _bounds(other._bounds)
        {

        }

        const T& content() const { return _content; }
        double area() const { return _area; }
        const Point& offset() const { return _offset; }
        const Rect& bounds() const { return _bounds; }
        const std::vector<Point>& convexHull() const { return _convexHull; }

        void setOffset(const Point& offset) {
            _offset = offset;
            _bounds.left += offset.x;
            _bounds.right += offset.x;
            _bounds.top += offset.y;
            _bounds.bottom += offset.y;

            // translate polygons
            for (auto it = _polygons.begin(); it != _polygons.end(); ++it) {
                for (auto it_p = (*it).begin(); it_p != (*it).end(); ++it_p) {
                    (*it_p).x += offset.x;
                    (*it_p).y += offset.y;
                }
            }

            // translate convexHull
            for (auto it_point = _convexHull.begin(); it_point != _convexHull.end(); ++it_point) {
                (*it_point).x += offset.x;
                (*it_point).y += offset.y;
            }

        }

    protected:
        T _content;
        Point _offset;
        std::vector<std::vector<Point>> _polygons;
        std::vector<Point> _convexHull;
        double _area;
        Rect _bounds;
    };

    template <class T> class ContentList: public std::vector<Content<T>> {
    public:
        ContentList<T>& operator += (const Content<T>& content) {
            this->push_back(content);
            return *this;
        }

        void sort() {
            std::sort(this->begin(), this->end(), [](const Content<T> &a, const Content<T> &b){
                auto areaA = a.area();
                auto areaB = b.area();
                return areaA > areaB;
            });
        }
    };

    template <class T> class Container: public std::vector<Content<T>> {
    public:
        void place(const ContentList<T>& inputContent, int step = 5) {
            for (auto it = inputContent.begin(); it != inputContent.end(); ++it) {
                auto content = (*it);
                qDebug() << "place:" << content.area();
                // insert first
                if (it == inputContent.begin()) {
                    _bounds = content.bounds();
                    _contentList.push_back(content);
                    _convexHull = content.convexHull();
                } else {
                    float startX = _bounds.left - (content.bounds().right - content.bounds().left) - step;
                    float startY = _bounds.top - (content.bounds().bottom - content.bounds().top) - step;
                    float endX = _bounds.right + step;
                    float endY = _bounds.bottom + step;

                    bool isPlaces = false;
                    float bestArea = 0;
                    Point bestOffset;
                    std::vector<Point> bestConvexHull;

                    for (float y = startY; y < endY; y+= step) {
                        for (float x = startX; x < endX; x+= step) {
                            auto contentConvexHull = content.convexHull();
                            for (auto it_point = contentConvexHull.begin(); it_point != contentConvexHull.end(); ++it_point) {
                                (*it_point).x += x;
                                (*it_point).y += y;
                            }

                            // test intersect intersection
                            bool intersect = false;
                            intersect = convexHullIntersect(contentConvexHull, _convexHull);
//                            for (auto in_it = _contentList.begin(); in_it != _contentList.end(); ++in_it) {
//                                if (convexHullIntersect(contentConvexHull, (*in_it).convexHull())) {
//                                    intersect = true;
//                                    break;
//                                }
//                            }
                            if (!intersect) {
                                auto newConvexHull = _convexHull;
                                newConvexHull.insert(newConvexHull.begin(), contentConvexHull.begin(), contentConvexHull.end());
                                float area = convexHullArea(newConvexHull);

                                if ((bestArea > area) || (!isPlaces)) {
                                    //qDebug() << area << bestArea;
                                    bestArea = area;
                                    bestOffset = Point(x, y);
                                    bestConvexHull = newConvexHull;
                                    isPlaces = true;
                                }
                            }
                        }
                    }

                    if (isPlaces) {
                        content.setOffset(bestOffset);
                        if (_bounds.left > content.bounds().left) _bounds.left = content.bounds().left;
                        if (_bounds.right < content.bounds().right) _bounds.right = content.bounds().right;
                        if (_bounds.top > content.bounds().top) _bounds.top = content.bounds().top;
                        if (_bounds.bottom < content.bounds().bottom) _bounds.bottom = content.bounds().bottom;
                        _contentList.push_back(content);
                        _convexHull = bestConvexHull;
                    } else {
                        qDebug() << "Not placed";
                    }

//                    if (placeVariant.size()) {
//                        content.setOffset((*placeVariant.begin()).second);
//                        if (_bounds.left > content.bounds().left) _bounds.left = content.bounds().left;
//                        if (_bounds.right < content.bounds().right) _bounds.right = content.bounds().right;
//                        if (_bounds.top > content.bounds().top) _bounds.top = content.bounds().top;
//                        if (_bounds.bottom < content.bounds().bottom) _bounds.bottom = content.bounds().bottom;
//                        _contentList.push_back(content);
//                    } else {
//                        qDebug() << "Not placed";
//                    }
                }
            }
        }

        const Rect& bounds() const { return _bounds; }
        const ContentList<T>& contentList() const { return _contentList; }
    protected:
        Rect _bounds;
        ContentList<T> _contentList;
        std::vector<Point> _convexHull;
    };

}

#endif // POLYPACK2D_H
