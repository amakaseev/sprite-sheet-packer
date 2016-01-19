#include "polypack2d.h"

namespace PolyPack2D {

    float cross(const Point &o, const Point &a, const Point &b) {
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
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

    float convexHullArea(std::vector<Point> p) {
        float area = 0;
        auto length = p.size();
        for(auto a = 0; a < length; a++) {
            int b = ((a+1) % length);
            area += (p[a].x * p[b].y) - (p[b].x * p[a].y);
        }
        return area;
    }

}
