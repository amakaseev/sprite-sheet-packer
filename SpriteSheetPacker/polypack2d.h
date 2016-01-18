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

#include "clipper.hpp"
#include <QPointF>
#include <math.h>
//#include <list>
//#include <algorithm>

namespace PolyPack2D {
    const float PRECISION = 10.f;

    template<class T> class Content {
    public:
        Content(const T &content, std::vector<std::vector<QPointF>> polygons)
        : _content(content)
        , _area(0)
        {
            // calculate area of polygons
            for (auto it = polygons.begin(); it != polygons.end(); ++it) {
                ClipperLib::Path poly;
                for (auto it_p = (*it).begin(); it_p != (*it).end(); ++it_p) {
                    poly << ClipperLib::IntPoint(it_p->x() * PRECISION, it_p->y() * PRECISION);
                }
                _area += fabs(ClipperLib::Area(poly));
                _polygons.push_back(poly);
            }

        }
        Content(const Content& other)
            : _content(other._content)
            , _polygons(other._polygons)
            , _area(other._area)
        {
            (*this) = other;
        }

        const T& content() const { return _content; }
        double area() const { return _area; }

    protected:
        T _content;
        std::vector<ClipperLib::Path> _polygons;
        double _area;
    };

    template <class T> class ContentList: public std::vector<Content<T>> {
    public:
        ContentList<T>& operator += (const Content<T>& content) {
            this->push_back(content);
            return *this;
        }

        void sort() {
            std::sort(this->begin(), this->end(), GreatestWidthThenGreatestAreaSort());
        }

    private:
        struct GreatestWidthThenGreatestAreaSort {
            bool operator()(const Content<T> &a, const Content<T> &b) const {
                auto areaA = a.area();
                auto areaB = b.area();

                return areaA > areaB;
            }
        };
    };

}

#endif // POLYPACK2D_H
