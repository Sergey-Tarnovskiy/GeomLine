#pragma once

#include <vector>
#include <chrono>
#include <cmath>
#include <omp.h>
#include "geometry.h"

struct GridResult {
    std::vector<Point> intersections;
    std::vector<RectArea> grid_cells;
    double elapsed_ms = 0.0;
};

class UniformGridSolver {
public:
    static GridResult solve(const std::vector<Segment>& segments, int num_threads) {
        GridResult result;
        if (segments.size() < 2) {
            return result;
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Размер регулярной сетки K x K ориентируется на число потоков
        int grid_dim = static_cast<int>(std::ceil(std::sqrt(num_threads)));
        if (grid_dim < 2) grid_dim = 2;

        int total_cells = grid_dim * grid_dim;
        result.grid_cells.resize(total_cells);

        double cell_w = 1.0 / grid_dim;
        double cell_h = 1.0 / grid_dim;

        for (int row = 0; row < grid_dim; ++row) {
            for (int col = 0; col < grid_dim; ++col) {
                int cell_idx = row * grid_dim + col;
                result.grid_cells[cell_idx] = RectArea{
                    col * cell_w, (col + 1) * cell_w,
                    row * cell_h, (row + 1) * cell_h
                };
            }
        }

        // Распределение отрезков по ячейкам сетки
        std::vector<std::vector<Segment>> cell_segments(total_cells);
        for (const auto& seg : segments) {
            for (int i = 0; i < total_cells; ++i) {
                if (result.grid_cells[i].intersects_segment(seg)) {
                    cell_segments[i].push_back(seg);
                }
            }
        }

        // Локальные буферы точек для каждого потока
        std::vector<std::vector<Point>> thread_points(num_threads);

        omp_set_num_threads(num_threads);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();

            #pragma omp for schedule(dynamic)
            for (int cell_idx = 0; cell_idx < total_cells; ++cell_idx) {
                const auto& cell = result.grid_cells[cell_idx];
                const auto& local_segs = cell_segments[cell_idx];
                size_t m = local_segs.size();

                bool is_right_edge = (cell_idx % grid_dim == grid_dim - 1);
                bool is_bottom_edge = (cell_idx / grid_dim == grid_dim - 1);

                for (size_t i = 0; i < m; ++i) {
                    for (size_t j = i + 1; j < m; ++j) {
                        auto pt = find_intersection(local_segs[i], local_segs[j]);
                        if (pt.has_value()) {
                            // Spatial Ownership: точка добавляется только если принадлежит текущей ячейке
                            if (cell.contains_point(pt.value(), is_right_edge, is_bottom_edge)) {
                                thread_points[tid].push_back(pt.value());
                            }
                        }
                    }
                }
            }
        }

        // Сборка точек без гонки данных
        for (const auto& local_pts : thread_points) {
            result.intersections.insert(result.intersections.end(), local_pts.begin(), local_pts.end());
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        result.elapsed_ms = duration.count();

        return result;
    }
};