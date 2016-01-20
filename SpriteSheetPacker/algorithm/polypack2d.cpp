#include "polypack2d.h"
#include "triangle_triangle_intersection.h"

namespace PolyPack2D {
    template<typename T> T max(T a, T b) {
        return (a > b)? a:b;
    }
    template<typename T> T min(T a, T b) {
        return (a < b)? a:b;
    }

    float cross(const Point &o, const Point &a, const Point &b) {
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
    }

    float dot2(const Point& vec1, const Point& vec2){
        return vec1.x * vec2.y - vec1.y * vec2.x;
    }

    float pointsArea(const std::vector<Point>& p) {
        Rect bounds;
        bounds.left = bounds.top = std::numeric_limits<float>::max();
        bounds.right = bounds.bottom = std::numeric_limits<float>::min();
        for (auto point: p) {
            if (bounds.left > point.x) bounds.left = point.x;
            if (bounds.right < point.x) bounds.right = point.x;
            if (bounds.top > point.y) bounds.top = point.y;
            if (bounds.bottom < point.y) bounds.bottom = point.y;
        }
        return bounds.area();
    }

    // Returns a list of points on the convex hull in counter-clockwise order.
    // Note: the last point in the returned list is the same as the first one.
    std::vector<Point> createConvexHull(const std::vector<Point>& points) {
        std::vector<Point> p = points;
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

    bool rectIntersect(const Rect& r1, const Rect& r2) {
        return !(r2.left > r1.right ||
                 r2.right < r1.left ||
                 r2.top > r1.bottom ||
                 r2.bottom < r1.top);
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

    // To find orientation of ordered triplet (p, q, r).
    // The function returns following values
    // 0 --> p, q and r are colinear
    // 1 --> Clockwise
    // 2 --> Counterclockwise
    int orientation(Point p, Point q, Point r) {
        // See http://www.geeksforgeeks.org/orientation-3-ordered-points/
        // for details of below formula.
        int val = (q.y - p.y) * (r.x - q.x) -
                  (q.x - p.x) * (r.y - q.y);

        if (val == 0) return 0;  // colinear

        return (val > 0)? 1: 2; // clock or counterclock wise
    }

    // Given three colinear points p, q, r, the function checks if
    // point q lies on line segment 'pr'
    bool onSegment(Point p, Point q, Point r) {
        if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
            q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
           return true;

        return false;
    }

    bool edgeIntersection(const Point& p1, const Point& q1, const Point& p2, const Point& q2) {
        // Find the four orientations needed for general and
        // special cases
        int o1 = orientation(p1, q1, p2);
        int o2 = orientation(p1, q1, q2);
        int o3 = orientation(p2, q2, p1);
        int o4 = orientation(p2, q2, q1);

        // General case
        if (o1 != o2 && o3 != o4)
            return true;

        // Special Cases
        // p1, q1 and p2 are colinear and p2 lies on segment p1q1
        if (o1 == 0 && onSegment(p1, p2, q1)) return true;

        // p1, q1 and p2 are colinear and q2 lies on segment p1q1
        if (o2 == 0 && onSegment(p1, q2, q1)) return true;

        // p2, q2 and p1 are colinear and p1 lies on segment p2q2
        if (o3 == 0 && onSegment(p2, p1, q2)) return true;

         // p2, q2 and q1 are colinear and q1 lies on segment p2q2
        if (o4 == 0 && onSegment(p2, q1, q2)) return true;

        return false; // Doesn't fall in any of the above cases
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
        size_t aLength = aConvexHull.size();
        size_t bLength = bConvexHull.size();
        for (size_t i = 0; i < aLength; ++i) {
            Point a1(aConvexHull[i]);
            Point a2(aConvexHull[(i == (aLength-1))? 0:(i+1)]);
            for (size_t j = 0; j < bLength; ++j) {
                Point b1(bConvexHull[j]);
                Point b2(bConvexHull[(j == (bLength-1))? 0:(j+1)]);
                if (edgeIntersection(a1, a2, b1, b2)) return true;
            }
        }

        return false;
    }

    bool trianglesIntersect(const Triangles& a, const Triangles& b) {
        for (int i=0; i<a.indices.size(); i+=3) {
            float a1[2] = { a.verts[a.indices[i+0]].x, a.verts[a.indices[i+0]].y };
            float a2[2] = { a.verts[a.indices[i+1]].x, a.verts[a.indices[i+1]].y };
            float a3[2] = { a.verts[a.indices[i+2]].x, a.verts[a.indices[i+2]].y };

            for (int j=0; j<b.indices.size(); j+=3) {
                float b1[2] = { b.verts[b.indices[j+0]].x, b.verts[b.indices[j+0]].y };
                float b2[2] = { b.verts[b.indices[j+1]].x, b.verts[b.indices[j+1]].y };
                float b3[2] = { b.verts[b.indices[j+2]].x, b.verts[b.indices[j+2]].y };

                if (tri_tri_overlap_test_2d(a1, a2, a3, b1, b2, b3)) {
                    return true;
                }
                if (tri_tri_overlap_test_2d(b1, b2, b3, a1, a2, a3)) {
                    return true;
                }
            }
        }
        return false;
    }

}
