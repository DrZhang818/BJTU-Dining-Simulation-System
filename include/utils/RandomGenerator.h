#pragma once

#include <cstdint>
#include <random>

namespace bdss::utils {

class RandomGenerator {
public:
    static void setSeed(std::uint32_t seed);
    static int getPoisson(double mean);
    static int getNormalInt(double mean, double stddev, int minimum = 1);
    static int getUniformInt(int minInclusive, int maxInclusive);
    static double getUniformDouble(double minInclusive, double maxInclusive);
    static int getWeightedChoice(const std::vector<double>& weights);

private:
    static std::mt19937& engine();
};

} // namespace bdss::utils
