#pragma once

#include <vector>
#include <chrono>
#include <cmath>
#include <omp.h>
#include "geometry.h"

// Uniform grid intersection search result.
struct GridResult {
    std::vector<Point> intersections; // One point per intersecting segment pair.
    std::vector<RectArea> grid_cells; // Grid cells.
    double elapsed_ms = 0.0;          // Elapsed time in milliseconds.
};

// Uniform grid intersection search.
class UniformGridSolver {
public:
    static GridResult solve(const std::vector<Segment>& segments, int num_threads) {
        GridResult result;
        if (segments.size() < 2) {
            return result;
        }
        num_threads = std::max(1, num_threads);

        auto start_time = std::chrono::high_resolution_clock::now();

        // Choose the grid dimensions from the thread count.
        int grid_dim = static_cast<int>(std::ceil(std::sqrt(num_threads)));
        if (grid_dim < 2) grid_dim = 2;

        // Create the grid and its cell boundaries.
        int total_cells = grid_dim * grid_dim;
        result.grid_cells.resize(total_cells);

        // Cell dimensions in the normalized domain.
        double cell_w = 1.0 / grid_dim;
        double cell_h = 1.0 / grid_dim;

        // Populate the grid with rectangular cells.
        for (int row = 0; row < grid_dim; ++row) {
            for (int col = 0; col < grid_dim; ++col) {
                // Flatten the row and column indices.
                int cell_idx = row * grid_dim + col;
                result.grid_cells[cell_idx] = RectArea{
                    col * cell_w, (col + 1) * cell_w,
                    row * cell_h, (row + 1) * cell_h
                };
            }
        }

        // Store the segments intersecting each cell.
        std::vector<std::vector<Segment>> cell_segments(total_cells);

        // Assign each segment to every cell it intersects.
        for (const auto& seg : segments) {
            for (int i = 0; i < total_cells; ++i) {
                if (result.grid_cells[i].intersects_segment(seg)) {
                    cell_segments[i].push_back(seg);
                }
            }
        }

        // Each thread collects points in its own buffer.
        std::vector<std::vector<Point>> thread_points(num_threads);

        omp_set_num_threads(num_threads);

        #pragma omp parallel
        {
            // Get the current thread index.
            int tid = omp_get_thread_num();

            #pragma omp for schedule(dynamic)
            for (int cell_idx = 0; cell_idx < total_cells; ++cell_idx) {
                // Get the segments intersecting this cell.
                const auto& cell = result.grid_cells[cell_idx];   // Grid cell.
                const auto& local_segs = cell_segments[cell_idx]; // Segments intersecting the cell.
                size_t m = local_segs.size();

                for (size_t i = 0; i < m; ++i) {
                    for (size_t j = i + 1; j < m; ++j) {
                        auto pt = find_intersection(local_segs[i], local_segs[j]);
                        if (pt.has_value()) {
                            // Only the owning cell records the point.
                            if (cell.contains_point(pt.value(),
                                    cell_idx % grid_dim == grid_dim - 1,
                                    cell_idx / grid_dim == grid_dim - 1)) {
                                thread_points[tid].push_back(pt.value());
                            }
                        }
                    }
                }
            }
        }

        // Merge buffers after all threads finish.
        for (const auto& local_pts : thread_points) {
            result.intersections.insert(result.intersections.end(), local_pts.begin(), local_pts.end());
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        result.elapsed_ms = duration.count();

        return result;
    }
};
