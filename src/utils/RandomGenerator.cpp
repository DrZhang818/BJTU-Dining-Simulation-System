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

} // namespace bdss::utils
