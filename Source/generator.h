#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include "geometry.h"

enum class DistributionType {
    Uniform,
    Clustered
};

class SegmentGenerator {
public:
    static std::vector<Segment> generate(int count, 
                                         DistributionType type = DistributionType::Uniform, 
                                         unsigned int seed = std::random_device{}()) 
    {
        std::mt19937 rng(seed);
        std::vector<Segment> segments;
        segments.reserve(count);

        if (type == DistributionType::Uniform) {
            generate_uniform(segments, count, rng);
        } else {
            generate_clustered(segments, count, rng);
        }

        return segments;
    }

private:
    static void generate_uniform(std::vector<Segment>& segments, int count, std::mt19937& rng) {
        std::uniform_real_distribution<double> pos_dist(0.05, 0.95);
        std::uniform_real_distribution<double> len_dist(0.02, 0.08);
        std::uniform_real_distribution<double> angle_dist(0.0, 2.0 * M_PI);

        for (int i = 0; i < count; ++i) {
            double cx = pos_dist(rng);
            double cy = pos_dist(rng);
            double len = len_dist(rng);
            double angle = angle_dist(rng);

            double dx = 0.5 * len * std::cos(angle);
            double dy = 0.5 * len * std::sin(angle);

            Point p1{std::clamp(cx - dx, 0.0, 1.0), std::clamp(cy - dy, 0.0, 1.0)};
            Point p2{std::clamp(cx + dx, 0.0, 1.0), std::clamp(cy + dy, 0.0, 1.0)};

            segments.push_back(Segment{i, p1, p2});
        }
    }

    static void generate_clustered(std::vector<Segment>& segments, int count, std::mt19937& rng) {
        // Задаем центры нескольких кластеров для сильного локального сгущения
        const std::vector<Point> cluster_centers = {
            {0.25, 0.30},
            {0.75, 0.70},
            {0.30, 0.75}
        };

        std::uniform_int_distribution<size_t> cluster_dist(0, cluster_centers.size() - 1);
        std::normal_distribution<double> offset_dist(0.0, 0.07);
        std::uniform_real_distribution<double> len_dist(0.02, 0.06);
        std::uniform_real_distribution<double> angle_dist(0.0, 2.0 * M_PI);

        for (int i = 0; i < count; ++i) {
            const Point& center = cluster_centers[cluster_dist(rng)];

            double cx = std::clamp(center.x + offset_dist(rng), 0.05, 0.95);
            double cy = std::clamp(center.y + offset_dist(rng), 0.05, 0.95);
            double len = len_dist(rng);
            double angle = angle_dist(rng);

            double dx = 0.5 * len * std::cos(angle);
            double dy = 0.5 * len * std::sin(angle);

            Point p1{std::clamp(cx - dx, 0.0, 1.0), std::clamp(cy - dy, 0.0, 1.0)};
            Point p2{std::clamp(cx + dx, 0.0, 1.0), std::clamp(cy + dy, 0.0, 1.0)};

            segments.push_back(Segment{i, p1, p2});
        }
    }
};