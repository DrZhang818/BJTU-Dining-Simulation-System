#pragma once
#include <random>

namespace bdss::utils {

class RandomGenerator {
public:
    RandomGenerator() = delete;
    static int getPoisson(double lambda);
    static double getNormal(double mean, double stddev);
    static int getUniformInt(int min, int max);

private:
    static std::mt19937& getEngine();
};

}  // namespace bdss::utils