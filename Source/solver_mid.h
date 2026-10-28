#pragma once

#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <omp.h>
#include "geometry.h"

struct RCBSolverResult {
    std::vector<Point> intersections;
    std::vector<RectArea> grid_cells;
    double elapsed_ms = 0.0;
};

class RCBSolver {
public:
    static RCBSolverResult solve(const std::vector<Segment>& segments, int num_threads) {
        RCBSolverResult result;
        if (segments.size() < 2) {
            return result;
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // 1. Определение числа целевых областей как степени двойки (>= num_threads)
        int target_parts = 1;
        while (target_parts < num_threads) {
            target_parts <<= 1;
        }
        if (target_parts < 2) target_parts = 2;

        // Копия массива для работы std::nth_element
        std::vector<Segment> segs_copy = segments;

        // 2. Рекурсивное бинарное деление пространства
        RectArea initial_domain{0.0, 1.0, 0.0, 1.0};
        split_recursive(segs_copy.begin(), segs_copy.end(), initial_domain, target_parts, result.grid_cells);

        size_t total_cells = result.grid_cells.size();

        // 3. Распределение отрезков по полученным адаптивным ячейкам
        std::vector<std::vector<Segment>> cell_segments(total_cells);
        for (const auto& seg : segments) {
            for (size_t i = 0; i < total_cells; ++i) {
                if (result.grid_cells[i].intersects_segment(seg)) {
                    cell_segments[i].push_back(seg);
                }
            }
        }

        // 4. Параллельный расчет пересечений внутри каждой ячейки
        std::vector<std::vector<Point>> thread_points(num_threads);
        omp_set_num_threads(num_threads);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();

            #pragma omp for schedule(dynamic)
            for (size_t cell_idx = 0; cell_idx < total_cells; ++cell_idx) {
                const auto& cell = result.grid_cells[cell_idx];
                const auto& local_segs = cell_segments[cell_idx];
                size_t m = local_segs.size();

                // Внешние границы всего пространства замкнуты
                bool is_right_edge = (std::abs(cell.x_max - 1.0) < 1e-7);
                bool is_bottom_edge = (std::abs(cell.y_max - 1.0) < 1e-7);

                for (size_t i = 0; i < m; ++i) {
                    for (size_t j = i + 1; j < m; ++j) {
                        auto pt = find_intersection(local_segs[i], local_segs[j]);
                        if (pt.has_value()) {
                            // Spatial Ownership: устранение дубликатов на стыках областей
                            if (cell.contains_point(pt.value(), is_right_edge, is_bottom_edge)) {
                                thread_points[tid].push_back(pt.value());
                            }
                        }
                    }
                }
            }
        }

        // 5. Объединение результатов работы потоков
        for (const auto& local_pts : thread_points) {
            result.intersections.insert(result.intersections.end(), local_pts.begin(), local_pts.end());
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        result.elapsed_ms = duration.count();

        return result;
    }

private:
    using SegIter = std::vector<Segment>::iterator;

    static void split_recursive(SegIter begin, SegIter end, const RectArea& box, int parts, std::vector<RectArea>& out_cells) {
        if (parts <= 1 || std::distance(begin, end) <= 1) {
            out_cells.push_back(box);
            return;
        }

        double width = box.x_max - box.x_min;
        double height = box.y_max - box.y_min;
        bool split_by_x = (width >= height);

        auto mid = begin + std::distance(begin, end) / 2;

        if (split_by_x) {
            std::nth_element(begin, mid, end, [](const Segment& a, const Segment& b) {
                return a.mid_point().x < b.mid_point().x;
            });

            double split_val = mid->mid_point().x;
            // Защита от вырождения ячейки, если все центры совпадают с краями
            split_val = std::clamp(split_val, box.x_min + 0.001, box.x_max - 0.001);

            RectArea left_box{box.x_min, split_val, box.y_min, box.y_max};
            RectArea right_box{split_val, box.x_max, box.y_min, box.y_max};

            split_recursive(begin, mid, left_box, parts / 2, out_cells);
            split_recursive(mid, end, right_box, parts - parts / 2, out_cells);
        } else {
            std::nth_element(begin, mid, end, [](const Segment& a, const Segment& b) {
                return a.mid_point().y < b.mid_point().y;
            });

            double split_val = mid->mid_point().y;
            split_val = std::clamp(split_val, box.y_min + 0.001, box.y_max - 0.001);

            RectArea top_box{box.x_min, box.x_max, box.y_min, split_val};
            RectArea bottom_box{box.x_min, box.x_max, split_val, box.y_max};

            split_recursive(begin, mid, top_box, parts / 2, out_cells);
            split_recursive(mid, end, bottom_box, parts - parts / 2, out_cells);
        }
    }
};