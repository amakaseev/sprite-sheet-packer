#include "polypack2d.h"

namespace PolyPack2D {

    float cross(const Point &o, const Point &a, const Point &b) {
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
    }

    float dot2(const Point& vec1, const Point& vec2){
        return vec1.x * vec2.y - vec1.y * vec2.x;
    }

    // Returns a list of points on the convex hull in counter-clockwise order.
    // Note: the last point in the returned list is the same as the first one.
    std::vector<Point> createConvexHull(std::vector<Point> p) {
        int n = p.size(), k = 0;
        std::vector<Point> result(2*n);

        // Sort points lexicographically
        std::sort(p.begin(), p.end(), [](const Point &a, const Point &b) {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
        });

        // Build lower hull
        for (int i = 0; i < n; ++i) {
            while (k >= 2 && cross(result[k-2], result[k-1], p[i]) <= 0) k--;
            result[k++] = p[i];
        }

        // Build upper hull
        for (int i = n-2, t = k+1; i >= 0; i--) {
            while (k >= t && cross(result[k-2], result[k-1], p[i]) <= 0) k--;
            result[k++] = p[i];
        }

        result.resize(k);
        return result;
    }

    float convexHullArea(const std::vector<Point>& p) {
        float area = 0;
        size_t length = p.size();
        for(size_t a = 0; a < length; a++) {
            int b = ((a+1) % length);
            area += (p[a].x * p[b].y) - (p[b].x * p[a].y);
        }
        return area;
    }

    bool pointInConvexHull(const Point& point, const std::vector<Point>& convexHull) {
        size_t length = convexHull.size();
        for (size_t i = 0; i < length; ++i) {
            Point v1(convexHull[i] - point);
            Point v2(convexHull[(i == (length-1))? 0:(i+1)] - point);
            Point edge(v1 - v2);

            if (dot2(edge, v1) < 0) {
                return false;
            }
        }
        return true;
    }

    //one edge is a-b, the other is c-d
    bool edgeIntersection(const Point& a, const Point& b, const Point& c, const Point& d){
        float det = dot2(b - a, c - d);
        float t   = dot2(c - a, c - d) / det;
        float u   = dot2(b - a, c - a) / det;
        if ((t < 0) || (u < 0) || (t > 1) || (u > 1)) {
            return false;
        } else {
            return true; // out = a * (1 - t) + t * b;
        }
    }

    bool convexHullIntersect(const std::vector<Point>& aConvexHull, const std::vector<Point>& bConvexHull) {
        // test all points
        for (auto point: aConvexHull) {
            if (pointInConvexHull(point, bConvexHull)) return true;
        }
        for (auto point: bConvexHull) {
            if (pointInConvexHull(point, aConvexHull)) return true;
        }

        // test edges
//        size_t aLength = aConvexHull.size();
//        size_t bLength = bConvexHull.size();
//        for (size_t i = 0; i < aLength; ++i) {
//            Point a1(aConvexHull[i]);
//            Point a2(aConvexHull[(i == (aLength-1))? 0:(i+1)]);
//            for (size_t j = 0; j < bLength; ++j) {
//                Point b1(bConvexHull[j]);
//                Point b2(bConvexHull[(j == (bLength-1))? 0:(j+1)]);
//                if (edgeIntersection(a1, a2, b1, b2)) return true;
//            }
//        }

        return false;
    }
}
