#include "utils/RandomGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace bdss::utils {

std::uint32_t RandomGenerator::seed_ = 42;

std::mt19937& RandomGenerator::engine() {
    static std::mt19937 mt(seed_);
    return mt;
}

void RandomGenerator::setSeed(std::uint32_t seed) {
    seed_ = seed;
    engine().seed(seed_);
}

int RandomGenerator::getPoisson(double lambda) {
    if (lambda <= 0.0) return 0;
    std::poisson_distribution<int> dist(lambda);
    return dist(engine());
}

int RandomGenerator::getNormalInt(double mean, double stddev, int minimum) {
    if (stddev <= std::numeric_limits<double>::epsilon()) {
        return std::max(minimum, static_cast<int>(std::lround(mean)));
    }
    std::normal_distribution<double> dist(mean, stddev);
    return std::max(minimum, static_cast<int>(std::lround(dist(engine()))));
}

int RandomGenerator::getUniformInt(int lowInclusive, int highInclusive) {
    if (highInclusive < lowInclusive) std::swap(lowInclusive, highInclusive);
    std::uniform_int_distribution<int> dist(lowInclusive, highInclusive);
    return dist(engine());
}

double RandomGenerator::getUniformDouble(double lowInclusive, double highExclusive) {
    if (highExclusive < lowInclusive) std::swap(lowInclusive, highExclusive);
    if (std::abs(highExclusive - lowInclusive) <= std::numeric_limits<double>::epsilon()) return lowInclusive;
    std::uniform_real_distribution<double> dist(lowInclusive, highExclusive);
    return dist(engine());
}

std::size_t RandomGenerator::getWeightedChoice(const std::vector<double>& weights) {
    if (weights.empty()) {
        throw std::invalid_argument("weighted choice requires at least one weight");
    }
    const double total = std::accumulate(weights.begin(), weights.end(), 0.0,
                                         [](double sum, double w) { return sum + std::max(0.0, w); });
    if (total <= std::numeric_limits<double>::epsilon()) {
        return 0;
    }
    std::uniform_real_distribution<double> dist(0.0, total);
    double r = dist(engine());
    for (std::size_t i = 0; i < weights.size(); ++i) {
        r -= std::max(0.0, weights[i]);
        if (r <= 0.0) return i;
    }
    return weights.size() - 1;
}

} // namespace bdss::utils
