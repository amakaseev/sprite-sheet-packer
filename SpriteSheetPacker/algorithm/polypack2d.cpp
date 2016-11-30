#include "polypack2d.h"
#include "triangle_triangle_intersection.h"

namespace PolyPack2D {
    bool rectIntersect(const Rect& r1, const Rect& r2) {
        return !(r2.left > r1.right ||
                 r2.right < r1.left ||
                 r2.top > r1.bottom ||
                 r2.bottom < r1.top);
    }

    bool trianglesIntersect(const Triangles& a, const Triangles& b) {
        for (size_t i=0; i<a.indices.size(); i+=3) {
            float a1[2] = { a.verts[a.indices[i+0]].x, a.verts[a.indices[i+0]].y };
            float a2[2] = { a.verts[a.indices[i+1]].x, a.verts[a.indices[i+1]].y };
            float a3[2] = { a.verts[a.indices[i+2]].x, a.verts[a.indices[i+2]].y };

            for (size_t j=0; j<b.indices.size(); j+=3) {
                float b1[2] = { b.verts[b.indices[j+0]].x, b.verts[b.indices[j+0]].y };
                float b2[2] = { b.verts[b.indices[j+1]].x, b.verts[b.indices[j+1]].y };
                float b3[2] = { b.verts[b.indices[j+2]].x, b.verts[b.indices[j+2]].y };

                if (tri_tri_overlap_test_2d(a1, a2, a3, b1, b2, b3)) {
                    return true;
                }
            }
        }
        return false;
    }
}
