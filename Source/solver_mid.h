#pragma once

#include <vector>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include "geometry.h"

struct SmartSolverResult {
    std::vector<Point> intersections; // One point per intersecting segment pair.
    std::vector<RectArea> grid_cells; // Partition strips.
    double elapsed_ms = 0.0;
};

class SmartSolver {
public:
    static SmartSolverResult solve(const std::vector<Segment>& segments, int num_threads) {
        SmartSolverResult result;
        const size_t seg_count = segments.size();
        if (seg_count < 2) return result;

        auto start_time = std::chrono::high_resolution_clock::now();
        // Number of strips.
        int p = std::max(1, num_threads);

        // Sort segment midpoints along the x axis.
        std::vector<Segment> sorted = segments;
        std::sort(sorted.begin(), sorted.end(), [](const Segment& a, const Segment& b) {
            return a.mid.x < b.mid.x;
        });

        // Partition into P strips using midpoint quantiles.
        result.grid_cells.resize(p);
        for (int i = 0; i < p; ++i) {
            // Choose strip boundaries from sorted midpoint coordinates
            // so each strip has approximately n/p segment midpoints.
            int idx_min = (seg_count * i) / p;
            int idx_max = (seg_count * (i + 1)) / p;
            double x_min = (i == 0) ? 0.0 : sorted[idx_min].mid.x;
            double x_max = (i == p - 1) ? 1.0 : sorted[idx_max].mid.x;
            result.grid_cells[i] = RectArea{x_min, x_max};
        }

        // Assign segments to every strip they overlap.
        std::vector<std::vector<Segment>> strip_segments(p);
        for (const auto& s : segments) {
            for (int i = 0; i < p; ++i) {
                if (s.max_x >= result.grid_cells[i].x_min && s.min_x <= result.grid_cells[i].x_max) {
                    strip_segments[i].push_back(s);
                }
            }
        }

        // Process strips in parallel with OpenMP.
        std::vector<std::vector<Point>> thread_points(p);
        omp_set_num_threads(p);

        #pragma omp parallel for schedule(dynamic)
        for (int c = 0; c < p; ++c) {
            int tid = omp_get_thread_num();
            const auto& strip = result.grid_cells[c];
            auto& segs = strip_segments[c];
            size_t m = segs.size();
            bool is_last = (c == p - 1);

            for (size_t i = 0; i < m; ++i) {
                for (size_t j = i + 1; j < m; ++j) {
                    auto pt = find_intersection(segs[i], segs[j]);
                    if (pt.has_value()) {
                        bool owns = (pt->x >= strip.x_min) && (is_last ? (pt->x <= strip.x_max) : (pt->x < strip.x_max));
                        if (owns) {
                            thread_points[tid].push_back(pt.value());
                        }
                    }
                }
            }
        }

        // Merge results.
        for (const auto& pts : thread_points) {
            result.intersections.insert(result.intersections.end(), pts.begin(), pts.end());
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        result.elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        return result;
    }
};
