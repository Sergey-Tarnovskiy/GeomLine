#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include "generator.h"
#include "solver_seg.h"
#include "solver_grid.h"
#include "solver_mid.h"

static void compare(std::vector<Point> expected, std::vector<Point> actual) {
    auto order = [](const Point& a, const Point& b) {
        return a.x == b.x ? a.y < b.y : a.x < b.x;
    };
    std::sort(expected.begin(), expected.end(), order);
    std::sort(actual.begin(), actual.end(), order);
    if (expected.size() != actual.size()) throw std::runtime_error("Intersection count mismatch");
    for (size_t i = 0; i < expected.size(); ++i) {
        if (std::abs(expected[i].x - actual[i].x) > 1e-8 ||
            std::abs(expected[i].y - actual[i].y) > 1e-8)
            throw std::runtime_error("Intersection coordinate mismatch");
    }
}

static void check(const std::vector<Segment>& segments) {
    auto expected = SequentialSolver::solve(segments).intersections;
    for (int threads : {-1, 0, 1, 2, 4, 8}) {
        compare(expected, UniformGridSolver::solve(segments, threads).intersections);
        compare(expected, SmartSolver::solve(segments, threads).intersections);
    }
}

int main() {
    try {
        check({});
        check({{{0, 0}, {1, 1}, 0}});
        for (double x : {0.0, 0.5 - 5e-10, 0.5, 0.5 + 5e-10, 1.0}) {
            std::vector<Segment> s = {{{x, 0}, {x, 1}, 0}, {{0, 0.5}, {1, 0.5}, 1}};
            if (SequentialSolver::solve(s).intersections.size() != 1)
                throw std::runtime_error("Missing crossing");
            check(s);
        }
        check({{{0.2, 0.8}, {0.5, 1}, 0}, {{0.8, 0.8}, {0.5, 1}, 1}});
        check({{{0.1, 0.3}, {0.8, 0.3}, 0}, {{0.2, 0.3}, {0.9, 0.3}, 1}});
        check({{{0.5, 0.5}, {0.5, 0.5}, 0}, {{0, 0.5}, {1, 0.5}, 1}});
        check({{{0, 0}, {1, 1}, 0}, {{0, 1}, {1, 0}, 1}, {{0, 0.5}, {1, 0.5}, 2}});
        for (auto distribution : {DistributionType::Uniform, DistributionType::Clustered})
            for (unsigned seed = 0; seed < 20; ++seed)
                check(SegmentGenerator::generate(200, distribution, seed));
        std::cout << "All solver checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
