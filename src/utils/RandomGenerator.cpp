#include "utils/RandomGenerator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace bdss::utils {

std::mt19937& RandomGenerator::engine() {
    static std::mt19937 instance{42U};
    return instance;
}

void RandomGenerator::setSeed(std::uint32_t seed) {
    engine().seed(seed);
}

int RandomGenerator::getPoisson(double mean) {
    if (mean <= 0.0) {
        return 0;
    }
    std::poisson_distribution<int> dist(mean);
    return dist(engine());
}

int RandomGenerator::getNormalInt(double mean, double stddev, int minimum) {
    if (stddev <= 0.0) {
        return std::max(minimum, static_cast<int>(std::lround(mean)));
    }
    std::normal_distribution<double> dist(mean, stddev);
    return std::max(minimum, static_cast<int>(std::lround(dist(engine()))));
}

int RandomGenerator::getUniformInt(int minInclusive, int maxInclusive) {
    if (minInclusive > maxInclusive) {
        throw std::invalid_argument("invalid uniform int range");
    }
    std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
    return dist(engine());
}

double RandomGenerator::getUniformDouble(double minInclusive, double maxInclusive) {
    if (minInclusive > maxInclusive) {
        throw std::invalid_argument("invalid uniform double range");
    }
    std::uniform_real_distribution<double> dist(minInclusive, maxInclusive);
    return dist(engine());
}

int RandomGenerator::getWeightedChoice(const std::vector<double>& weights) {
    if (weights.empty()) {
        throw std::invalid_argument("weights must not be empty");
    }
    double sum = 0.0;
    for (const auto w : weights) {
        if (w < 0.0) {
            throw std::invalid_argument("weights must be non-negative");
        }
        sum += w;
    }
    if (sum <= 0.0) {
        throw std::invalid_argument("sum of weights must be positive");
    }
    std::uniform_real_distribution<double> dist(0.0, sum);
    const double sample = dist(engine());
    double cumulative = 0.0;
    for (int i = 0; i < static_cast<int>(weights.size()); ++i) {
        cumulative += weights[i];
        if (sample < cumulative) {
            return i;
        }
    }
    return static_cast<int>(weights.size()) - 1;
}

} // namespace bdss::utils
