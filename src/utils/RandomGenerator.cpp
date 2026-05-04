#include "utils/RandomGenerator.h"

#include <algorithm>
#include <cmath>

namespace bdss::utils {

std::mt19937& RandomGenerator::getEngine() {
    static thread_local std::mt19937 engine(42);
    return engine;
}

void RandomGenerator::setSeed(unsigned int seed) {
    getEngine().seed(seed);
}

int RandomGenerator::getPoisson(double lambda) {
    if (lambda <= 0.0) {
        return 0;
    }

    std::poisson_distribution<int> distribution(lambda);
    return distribution(getEngine());
}

double RandomGenerator::getNormal(double mean, double stddev) {
    if (stddev <= 0.0) {
        return mean;
    }

    std::normal_distribution<double> distribution(mean, stddev);
    return std::max(0.0, distribution(getEngine()));
}

int RandomGenerator::getUniformInt(int min, int max) {
    if (min > max) {
        std::swap(min, max);
    }

    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(getEngine());
}

double RandomGenerator::getUniformReal(double min, double max) {
    if (min > max) {
        std::swap(min, max);
    }

    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(getEngine());
}

} // namespace bdss::utils