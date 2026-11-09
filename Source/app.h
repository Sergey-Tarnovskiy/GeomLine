#pragma once

#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <utility>
#include "geometry.h"
#include "generator.h"
#include "solver_seg.h"
#include "solver_grid.h"
#include "solver_mid.h"
#include <imgui.h>

struct AppConsole {
    std::vector<std::string> lines;
    bool auto_scroll = true;

    void add_log(const std::string& text) {
        lines.push_back(text);
    }

    void clear() {
        lines.clear();
    }

    void draw(const char* title, bool* p_open = nullptr) {
        if (!ImGui::Begin(title, p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Clear Logs")) {
            clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& line : lines) {
            ImGui::TextUnformatted(line.c_str());
        }

        if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
    }
};

struct AppState {
    int segment_count = 100;
    int thread_count = 4;
    int distribution_mode = 0;
    std::vector<Segment> segments;
    std::vector<Point> intersection_points;
    std::vector<RectArea> debug_grid;
    AppConsole console;

    int bench_n_start = 500;
    int bench_n_end = 3000;
    int bench_n_step = 500;
    int bench_threads_max = 8;

    void run_threads_scaling_benchmark() {
        if (segments.empty()) {
            console.add_log("[Warning] Generate segments before running benchmark!");
            return;
        }

        console.add_log("==========================================================================");
        console.add_log("[Scaling Benchmark] Variable Threads, Fixed Segments: " + std::to_string(segments.size()));

        auto seq_res = SequentialSolver::solve(segments);
        double t_seq = seq_res.elapsed_ms;
        size_t base_pts = seq_res.intersections.size();

        std::ostringstream ss_head;
        ss_head << std::fixed << std::setprecision(2);
        ss_head << "[Seq Base] Time: " << t_seq << " ms | Intersections: " << base_pts;
        console.add_log(ss_head.str());
        console.add_log("----------------------------------------");
        console.add_log(" Th | Grid Time (ms)  | Smart Time (ms)|");
        console.add_log("----+-----------------+----------------+");

        std::vector<int> thread_levels;
        for (int t = 1; t <= bench_threads_max; t = (t == 1 ? 2 : t + 2)) {
            thread_levels.push_back(t);
        }
        if (thread_levels.back() != bench_threads_max && bench_threads_max > 1) {
            thread_levels.push_back(bench_threads_max);
        }

        for (int t : thread_levels) {
            auto g_res = UniformGridSolver::solve(segments, t);
            auto smart_res = SmartSolver::solve(segments, t);


            std::ostringstream ss_row;
            ss_row << std::fixed << std::setprecision(2);
            ss_row << " " << std::setw(2) << t << " | "
                   << std::setw(14) << g_res.elapsed_ms << " | "
                   << std::setw(14) << smart_res.elapsed_ms << " | ";

            if (g_res.intersections.size() != base_pts || smart_res.intersections.size() != base_pts) {
                ss_row << " [WARN: DIFF]";
            }

            console.add_log(ss_row.str());
        }
        console.add_log("============================================================");
    }

    void run_size_scaling_benchmark() {
        console.add_log("===========================================================");
        console.add_log("[Size Benchmark] Variable Segments, Fixed Threads: " + std::to_string(thread_count));
        console.add_log("    N | Seq Time (ms)  | Grid Time (ms) | Smart Time (ms)  ");
        console.add_log("------+----------------+----------------+------------------");

        DistributionType type = (distribution_mode == 0) ? DistributionType::Uniform : DistributionType::Clustered;

        for (int n = bench_n_start; n <= bench_n_end; n += bench_n_step) {
            auto test_segs = SegmentGenerator::generate(n, type);

            auto seq_res = SequentialSolver::solve(test_segs);

            auto g_res = UniformGridSolver::solve(test_segs, thread_count);

            auto smart_res = SmartSolver::solve(test_segs, thread_count);

            std::ostringstream ss_row;
            ss_row << std::fixed << std::setprecision(2);
            ss_row << " " << std::setw(4) << n << " | "
                   << std::setw(14) << seq_res.elapsed_ms << " | "
                   << std::setw(14) << g_res.elapsed_ms << " | "
                   << std::setw(14) << smart_res.elapsed_ms << " | ";
            console.add_log(ss_row.str());
        }
        console.add_log("==========================================================");
    }

    void init() {
        console.add_log("[System] Application initialized successfully.");
    }

    void draw_control_panel() {
        ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::SliderInt("Segment Count", &segment_count, 10, 5000);
        ImGui::SliderInt("Threads (OMP)", &thread_count, 1, 16);

        const char* dist_names[] = { "Uniform Distribution", "Clustered Distribution" };
        ImGui::Combo("Distribution", &distribution_mode, dist_names, IM_ARRAYSIZE(dist_names));

        float btn_w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (ImGui::Button("Generate Segments", ImVec2(btn_w, 0))) {
            segments.clear();
            intersection_points.clear();
            debug_grid.clear();

            DistributionType type = (distribution_mode == 0) ? DistributionType::Uniform : DistributionType::Clustered;
            segments = SegmentGenerator::generate(segment_count, type);

            console.add_log(std::string("[Generation] Created ") +
                            std::to_string(segments.size()) + " segments (" +
                            dist_names[distribution_mode] + ").");
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Segments", ImVec2(btn_w, 0))) {
            segments.clear();
            intersection_points.clear();
            debug_grid.clear();
            console.add_log("[Cleanup] All data cleared.");
        }

        ImGui::SeparatorText("Algorithms");

        if (ImGui::Button("Sequential Algorithm", ImVec2(-1, 32))) {
            debug_grid.clear();
            intersection_points.clear();

            if (segments.empty()) {
                console.add_log("[Warning] Generate segments first!");
            } else {
                auto res = SequentialSolver::solve(segments);
                intersection_points = std::move(res.intersections);

                std::ostringstream ss;
                ss << std::fixed << std::setprecision(3);
                ss << "[Sequential] Found: " << intersection_points.size()
                   << " points | Time: " << res.elapsed_ms << " ms";
                console.add_log(ss.str());
            }
        }

        if (ImGui::Button("Uniform Grid", ImVec2(-1, 32))) {
            debug_grid.clear();
            intersection_points.clear();

            if (segments.empty()) {
                console.add_log("[Warning] Generate segments first!");
            } else {
                auto res = UniformGridSolver::solve(segments, thread_count);
                intersection_points = std::move(res.intersections);
                debug_grid = std::move(res.grid_cells);

                std::ostringstream ss;
                ss << std::fixed << std::setprecision(3);
                ss << "[Uniform Grid] Found: " << intersection_points.size()
                   << " points | Cells: " << debug_grid.size()
                   << " | Time: " << res.elapsed_ms << " ms";
                console.add_log(ss.str());
            }
        }

        if (ImGui::Button("Smart Partitioning", ImVec2(-1, 32))) {
            debug_grid.clear();
            intersection_points.clear();

            if (segments.empty()) {
                console.add_log("[Warning] Generate segments first!");
            } else {
                auto res = SmartSolver::solve(segments, thread_count);
                intersection_points = std::move(res.intersections);
                debug_grid = std::move(res.grid_cells);

                std::ostringstream ss;
                ss << std::fixed << std::setprecision(3);
                ss << "[Smart Solver] Found: " << intersection_points.size()
                   << " points | Cells: " << debug_grid.size()
                   << " | Time: " << res.elapsed_ms << " ms";
                console.add_log(ss.str());
            }
        }

        ImGui::SeparatorText("Benchmarking");

        if (ImGui::Button("Quick Compare (Current Setup)", ImVec2(-1, 30))) {
            if (segments.empty()) {
                console.add_log("[Warning] Generate segments first!");
            } else {
                auto seq_res = SequentialSolver::solve(segments);
                auto g_res = UniformGridSolver::solve(segments, thread_count);
                auto sm_res = SmartSolver::solve(segments, thread_count);

                std::ostringstream ss;
                ss << std::fixed << std::setprecision(2);
                ss << "[Quick] Seg: " << seq_res.elapsed_ms << "ms | "
                   << "Grid: " << g_res.elapsed_ms << "ms | "
                   << "Smart: " << sm_res.elapsed_ms << "ms | ";
                console.add_log(ss.str());

                intersection_points = std::move(sm_res.intersections);
                debug_grid = std::move(sm_res.grid_cells);
            }
        }

        ImGui::SliderInt("Max Threads", &bench_threads_max, 2, 16);
        if (ImGui::Button("Benchmark: Threads Scaling", ImVec2(-1, 30))) {
            run_threads_scaling_benchmark();
        }

        ImGui::InputInt("N Start", &bench_n_start, 250);
        ImGui::InputInt("N End", &bench_n_end, 500);
        ImGui::InputInt("N Step", &bench_n_step, 250);

        if (bench_n_start < 100) bench_n_start = 100;
        if (bench_n_step < 50) bench_n_step = 50;
        if (bench_n_end <= bench_n_start) bench_n_end = bench_n_start + bench_n_step;

        if (ImGui::Button("Benchmark: Size (N) Scaling", ImVec2(-1, 30))) {
            run_size_scaling_benchmark();
        }

        ImGui::End();
    }

    void draw_canvas() {
        ImGui::Begin("Geometric Canvas", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();

        // Fill the window area with a four-pixel inset.
        float side_x = canvas_sz.x - 4.0f;
        float side_y = canvas_sz.y - 4.0f;
        ImVec2 canvas_p1 = ImVec2(canvas_p0.x + side_x, canvas_p0.y + side_y);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(20, 20, 24, 255));
        draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(180, 180, 180, 255), 0.0f, 0, 1.5f);

        auto to_screen = [&](const Point& pt) -> ImVec2 {
            return ImVec2(
                canvas_p0.x + static_cast<float>(pt.x) * side_x,
                canvas_p0.y + static_cast<float>(pt.y) * side_y
            );
        };

        for (const auto& box : debug_grid) {
            ImVec2 b_min = to_screen({box.x_min, box.y_min});
            ImVec2 b_max = to_screen({box.x_max, box.y_max});
            draw_list->AddRect(b_min, b_max, IM_COL32(65, 180, 255, 200), 0.0f, 0, 2.0f);
        }

        for (const auto& s : segments) {
            draw_list->AddLine(to_screen(s.p1), to_screen(s.p2), IM_COL32(230, 230, 230, 140), 1.0f);
        }

        for (const auto& pt : intersection_points) {
            draw_list->AddCircleFilled(to_screen(pt), 3.5f, IM_COL32(255, 50, 50, 255));
        }

        ImGui::End();
    }

    void draw_ui() {
        ImGuiIO& io = ImGui::GetIO();
        float screen_w = io.DisplaySize.x;
        float screen_h = io.DisplaySize.y;

        float pad = 10.0f;
        float left_panel_w = 460.0f; // Left column width.

        float control_h = 570.0f;
        float console_h = screen_h - control_h - 3.0f * pad;

        // Control panel.
        ImGui::SetNextWindowPos(ImVec2(pad, pad), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(left_panel_w, control_h), ImGuiCond_Always);
        draw_control_panel();

        // Console output.
        ImGui::SetNextWindowPos(ImVec2(pad, pad * 2.0f + control_h), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(left_panel_w, console_h), ImGuiCond_Always);
        console.draw("Console Output");

        // The canvas fills the remaining window area.
        float canvas_x = left_panel_w + 2.0f * pad;
        float canvas_w = screen_w - canvas_x - pad;
        float canvas_h = screen_h - 2.0f * pad;

        ImGui::SetNextWindowPos(ImVec2(canvas_x, pad), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(canvas_w, canvas_h), ImGuiCond_Always);
        draw_canvas();
    }
};
