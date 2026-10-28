#include "geometry.h"

static constexpr double EPS = 1e-9;

double cross_product(const Point& a, const Point& b, const Point& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static bool on_segment_bounding_box(const Point& p, const Point& a, const Point& b) {
    return p.x >= std::min(a.x, b.x) - EPS && p.x <= std::max(a.x, b.x) + EPS &&
           p.y >= std::min(a.y, b.y) - EPS && p.y <= std::max(a.y, b.y) + EPS;
}

std::optional<Point> find_intersection(const Segment& s1, const Segment& s2) {
    // Fast AABB bounding box check
    if (s1.max_x() < s2.min_x() - EPS || s1.min_x() > s2.max_x() + EPS ||
        s1.max_y() < s2.min_y() - EPS || s1.min_y() > s2.max_y() + EPS) {
        return std::nullopt;
    }

    const Point& a = s1.p1;
    const Point& b = s1.p2;
    const Point& c = s2.p1;
    const Point& d = s2.p2;

    double cp1 = cross_product(a, b, c);
    double cp2 = cross_product(a, b, d);
    double cp3 = cross_product(c, d, a);
    double cp4 = cross_product(c, d, b);

    // General proper intersection
    if (((cp1 > EPS && cp2 < -EPS) || (cp1 < -EPS && cp2 > EPS)) &&
        ((cp3 > EPS && cp4 < -EPS) || (cp3 < -EPS && cp4 > EPS))) {
        
        // Line intersection calculation: P = a + t * (b - a)
        double denom = (b.x - a.x) * (d.y - c.y) - (b.y - a.y) * (d.x - c.x);
        if (std::abs(denom) < EPS) {
            return std::nullopt;
        }

        double t = ((c.x - a.x) * (d.y - c.y) - (c.y - a.y) * (d.x - c.x)) / denom;
        return Point{a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
    }

    // Collinear or endpoint cases
    if (std::abs(cp1) <= EPS && on_segment_bounding_box(c, a, b)) return c;
    if (std::abs(cp2) <= EPS && on_segment_bounding_box(d, a, b)) return d;
    if (std::abs(cp3) <= EPS && on_segment_bounding_box(a, c, d)) return a;
    if (std::abs(cp4) <= EPS && on_segment_bounding_box(b, c, d)) return b;

    return std::nullopt;
}

bool RectArea::contains_point(const Point& pt, bool is_right_edge, bool is_bottom_edge) const {
    bool in_x = is_right_edge ? (pt.x >= x_min - EPS && pt.x <= x_max + EPS)
                              : (pt.x >= x_min - EPS && pt.x < x_max - EPS);

    bool in_y = is_bottom_edge ? (pt.y >= y_min - EPS && pt.y <= y_max + EPS)
                               : (pt.y >= y_min - EPS && pt.y < y_max - EPS);

    return in_x && in_y;
}

bool RectArea::intersects_segment(const Segment& s) const {
    // AABB rejection test
    if (s.max_x() < x_min || s.min_x() > x_max) return false;
    if (s.max_y() < y_min || s.min_y() > y_max) return false;

    // Both endpoints inside
    auto inside = [this](const Point& p) {
        return p.x >= x_min && p.x <= x_max && p.y >= y_min && p.y <= y_max;
    };
    if (inside(s.p1) || inside(s.p2)) return true;

    // Check intersection with all 4 rectangle borders
    Point p_tl{x_min, y_min};
    Point p_tr{x_max, y_min};
    Point p_bl{x_min, y_max};
    Point p_br{x_max, y_max};

    Segment top{0, p_tl, p_tr};
    Segment bottom{0, p_bl, p_br};
    Segment left{0, p_tl, p_bl};
    Segment right{0, p_tr, p_br};

    return find_intersection(s, top).has_value() ||
           find_intersection(s, bottom).has_value() ||
           find_intersection(s, left).has_value() ||
           find_intersection(s, right).has_value();
}