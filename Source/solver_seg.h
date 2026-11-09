#pragma once

#include <vector>
#include <chrono>
#include "geometry.h"


// Sequential intersection search result.
struct SequentialResult {
    std::vector<Point> intersections; // One point per intersecting segment pair.
    double elapsed_ms = 0.0;          // Elapsed time in milliseconds.
};

// Sequential all-pairs intersection search.
class SequentialSolver {
public:
    static SequentialResult solve(const std::vector<Segment>& segments) {
        SequentialResult result;
        size_t n = segments.size();
        if (n < 2) {
            return result;
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Test every pair of segments.
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                auto pt = find_intersection(segments[i], segments[j]);
                if (pt.has_value()) {
                    result.intersections.push_back(pt.value());
                }
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        result.elapsed_ms = duration.count();

        return result;
    }
};
