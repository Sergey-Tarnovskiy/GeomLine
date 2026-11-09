#pragma once

#include <optional>
#include <algorithm>

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct Segment {
    int id;
    Point p1;
    Point p2;
    Point mid;
    double min_x;
    double max_x;
    double min_y;
    double max_y;

    Segment(const Point& point1, const Point& point2, int segment_id)
    :   id(segment_id),
        p1(point1),
        p2(point2),
          mid{(p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0},
          min_x(std::min(p1.x, p2.x)),
          max_x(std::max(p1.x, p2.x)),
          min_y(std::min(p1.y, p2.y)),
          max_y(std::max(p1.y, p2.y)) {}
};

struct RectArea {
    double x_min = 0.0;
    double x_max = 1.0;
    double y_min = 0.0;
    double y_max = 1.0;

    // Test whether pt belongs to this area.
    bool contains_point(const Point& pt, bool include_right = false, bool include_bottom = false) const;

    // Test whether s intersects this rectangle.
    bool intersects_segment(const Segment& s) const;
};

// Compute the cross product of vectors AB and AC.
double cross_product(const Point& a, const Point& b, const Point& c);

// Return one intersection point, if any.
std::optional<Point> find_intersection(const Segment& s1, const Segment& s2);
