#include "geometry.h"
#include <cmath>

static constexpr double EPS = 1e-9;

// Positive: c is to the left of ab.
// Negative: c is to the right of ab.
// Zero: a, b and c are collinear.
double cross_product(const Point& a, const Point& b, const Point& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Test whether p lies in the bounding box of ab.
static bool on_segment_bounding_box(const Point& p, const Point& a, const Point& b) {
    return p.x >= std::min(a.x, b.x) - EPS && p.x <= std::max(a.x, b.x) + EPS &&
           p.y >= std::min(a.y, b.y) - EPS && p.y <= std::max(a.y, b.y) + EPS;
}

// Return one intersection point, if any.
std::optional<Point> find_intersection(const Segment& s1, const Segment& s2) {
    // Reject disjoint bounding boxes.
    // Separated along either axis means no intersection.
    if (s1.max_x < s2.min_x - EPS || s1.min_x > s2.max_x + EPS ||
        s1.max_y < s2.min_y - EPS || s1.min_y > s2.max_y + EPS) {
        return std::nullopt;
    }

    const Point& a = s1.p1;
    const Point& b = s1.p2;
    const Point& c = s2.p1;
    const Point& d = s2.p2;

    double cp1 = cross_product(a, b, c); // c relative to ab
    double cp2 = cross_product(a, b, d); // d relative to ab
    double cp3 = cross_product(c, d, a); // a relative to cd
    double cp4 = cross_product(c, d, b); // b relative to cd

    // Test for a proper crossing.
    // Opposite signs on both segments indicate a crossing.
    if (((cp1 > EPS && cp2 < -EPS) || (cp1 < -EPS && cp2 > EPS)) &&
        ((cp3 > EPS && cp4 < -EPS) || (cp3 < -EPS && cp4 > EPS))) {

        // Compute the intersection using parametric coordinates.
        double denom = (b.x - a.x) * (d.y - c.y) - (b.y - a.y) * (d.x - c.x);
        if (std::abs(denom) < EPS) {
            return std::nullopt;
        }
        double t = ((c.x - a.x) * (d.y - c.y) - (c.y - a.y) * (d.x - c.x)) / denom;
        return Point{a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
    }

    // Handle shared endpoints and collinear overlap.
    if (std::abs(cp1) <= EPS && on_segment_bounding_box(c, a, b)) return c;
    if (std::abs(cp2) <= EPS && on_segment_bounding_box(d, a, b)) return d;
    if (std::abs(cp3) <= EPS && on_segment_bounding_box(a, c, d)) return a;
    if (std::abs(cp4) <= EPS && on_segment_bounding_box(b, c, d)) return b;

    // The segments do not intersect.
    return std::nullopt;
}

bool RectArea::contains_point(const Point& pt, bool include_right, bool include_bottom) const {
    bool in_x = include_right ? (pt.x >= x_min && pt.x <= x_max + EPS)
                              : (pt.x >= x_min && pt.x < x_max);

    bool in_y = include_bottom ? (pt.y >= y_min && pt.y <= y_max + EPS)
                               : (pt.y >= y_min && pt.y < y_max);

    return in_x && in_y;
}

// Test whether s intersects this rectangle.
bool RectArea::intersects_segment(const Segment& s) const {
    // Reject disjoint bounding boxes.
    if (s.max_x < x_min || s.min_x > x_max) return false;
    if (s.max_y < y_min || s.min_y > y_max) return false;

    // An endpoint inside the rectangle is sufficient.

    if ( s.p1.x >= x_min && s.p1.x <= x_max && s.p1.y >= y_min && s.p1.y <= y_max ) return true;

    // Otherwise test the rectangle edges.
    Point p1{x_min, y_min};
    Point p2{x_max, y_min};
    Point p3{x_min, y_max};
    Point p4{x_max, y_max};

    Segment top{p1, p2, 0};
    Segment bottom{p3, p4, 0};
    Segment left{p1, p3, 0};
    Segment right{p2, p4, 0};

    return find_intersection(s, top).has_value() ||
           find_intersection(s, bottom).has_value() ||
           find_intersection(s, left).has_value() ||
           find_intersection(s, right).has_value();
}
