#pragma once

#include <random>

namespace bdss::utils {

class RandomGenerator {
public:
    RandomGenerator() = delete;

    static void setSeed(unsigned int seed);
    static int getPoisson(double lambda);
    static double getNormal(double mean, double stddev);
    static int getUniformInt(int min, int max);
    static double getUniformReal(double min, double max);

private:
    static std::mt19937& getEngine();
};

} // namespace bdss::utils