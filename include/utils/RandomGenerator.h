#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace bdss::utils {

class RandomGenerator {
public:
    static void setSeed(std::uint32_t seed);
    static std::uint32_t currentSeed() noexcept { return seed_; }

    static int getPoisson(double lambda);
    static int getNormalInt(double mean, double stddev, int minimum = 1);
    static int getUniformInt(int lowInclusive, int highInclusive);
    static double getUniformDouble(double lowInclusive, double highExclusive);
    static std::size_t getWeightedChoice(const std::vector<double>& weights);

private:
    static std::mt19937& engine();
    static std::uint32_t seed_;
};

} // namespace bdss::utils
