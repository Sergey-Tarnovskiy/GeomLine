#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <numbers>
#include "geometry.h"

// Uniform or clustered segment distribution.
enum class DistributionType {
    Uniform,
    Clustered
};

// Generate segments with the selected distribution.
class SegmentGenerator {
public:
    static std::vector<Segment> generate(int count,
                                         DistributionType type = DistributionType::Uniform,
                                         unsigned int seed = std::random_device{}())
    {
        if (count <= 0) return {};

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
    // Create a segment from its center, length and angle.
    static Segment make_segment(int id, double x, double y, double length, double angle) {
        // Compute half-length projections onto each axis.
        const double dx = length * 0.5 * std::cos(angle);
        const double dy = length * 0.5 * std::sin(angle);

        return {
            {std::clamp(x - dx, 0.0, 1.0), std::clamp(y - dy, 0.0, 1.0)},
            {std::clamp(x + dx, 0.0, 1.0), std::clamp(y + dy, 0.0, 1.0)},
            id
        };
    }

    // Generate uniformly distributed segments.
    static void generate_uniform(std::vector<Segment>& segments, int count, std::mt19937& rng) {
        // Keep centers away from the domain boundary.
        std::uniform_real_distribution<double> pos_dist(0.05, 0.95);
        std::uniform_real_distribution<double> len_dist(0.02, 0.08);
        std::uniform_real_distribution<double> angle_dist(0.0, 2.0 * std::numbers::pi);

        for (int i = 0; i < count; ++i) {
            const double x = pos_dist(rng);
            const double y = pos_dist(rng);
            const double length = len_dist(rng);
            const double angle = angle_dist(rng);
            segments.push_back(make_segment(i, x, y, length, angle));
        }
    }

    // Generate clustered segments.
    static void generate_clustered(std::vector<Segment>& segments, int count, std::mt19937& rng) {

        std:: uniform_real_distribution<double> pos_dist(0.05, 0.95);
        const std::vector<Point> cluster_centers = {
            {pos_dist(rng), pos_dist(rng)},
            {pos_dist(rng), pos_dist(rng)},
            {pos_dist(rng), pos_dist(rng)}
        };

        // Choose a cluster and an offset within it.
        std::uniform_int_distribution<size_t> cluster_dist(0, cluster_centers.size() - 1);
        std::normal_distribution<double> offset_dist(0.0, 0.1);
        std::uniform_real_distribution<double> len_dist(0.02, 0.06);
        std::uniform_real_distribution<double> angle_dist(0.0, 2.0 * std::numbers::pi);

        for (int i = 0; i < count; ++i) {
            const Point& center = cluster_centers[cluster_dist(rng)];

            // Sample a normally distributed offset from the cluster center.
            const double x = std::clamp(center.x + offset_dist(rng), 0.05, 0.95);
            const double y = std::clamp(center.y + offset_dist(rng), 0.05, 0.95);
            const double length = len_dist(rng);
            const double angle = angle_dist(rng);
            segments.push_back(make_segment(i, x, y, length, angle));
        }
    }
};
