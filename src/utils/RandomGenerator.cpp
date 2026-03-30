#include "RandomGenerator.h"

#include <algorithm>
#include <cmath>

namespace bdss::utils {

std::mt19937& RandomGenerator::getEngine() {
    thread_local std::random_device rd;
    thread_local std::mt19937 engine(rd());
    return engine;
}

int RandomGenerator::getPoisson(double lambda) {
    if (lambda <= 0.0) return 0;
    std::poisson_distribution<int> dist(lambda);
    return dist(getEngine());
}

double RandomGenerator::getNormal(double mean, double stddev) {
    if (stddev <= 0.0) return mean;
    std::normal_distribution<double> dist(mean, stddev);
    return std::max(0.0, dist(getEngine()));
}

int RandomGenerator::getUniformInt(int min, int max) {
    if (min > max) std::swap(min, max);
    std::uniform_int_distribution<int> dist(min, max);
    return dist(getEngine());
}

}  // namespace bdss::utils