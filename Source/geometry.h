#pragma once

#include <optional>
#include <algorithm>
#include <cmath>

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct Segment {
    int id = 0;
    Point p1;
    Point p2;

    double min_x() const { return std::min(p1.x, p2.x); }
    double max_x() const { return std::max(p1.x, p2.x); }
    double min_y() const { return std::min(p1.y, p2.y); }
    double max_y() const { return std::max(p1.y, p2.y); }
    Point mid_point() const { return {(p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0}; }
};

struct RectArea {
    double x_min = 0.0;
    double x_max = 1.0;
    double y_min = 0.0;
    double y_max = 1.0;

    // Spatial Ownership: semi-open interval [x_min, x_max) x [y_min, y_max)
    // For outer system borders right and bottom bounds can be closed
    bool contains_point(const Point& pt, bool is_right_edge = false, bool is_bottom_edge = false) const;

    // AABB intersection check
    bool intersects_segment(const Segment& s) const;
};

// Vector cross product: (b - a) x (c - a)
double cross_product(const Point& a, const Point& b, const Point& c);

// Finds segment intersection point if it exists
std::optional<Point> find_intersection(const Segment& s1, const Segment& s2);