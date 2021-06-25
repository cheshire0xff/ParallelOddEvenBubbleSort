#ifndef INC_RANDOM_GENERATOR_H__
#define INC_RANDOM_GENERATOR_H__

#include <random>

class RandomGenerator {
   public:
    RandomGenerator(int min, int max)
        : gen(std::random_device{}()), distribution(min, max) {}
    int generate() { return distribution(gen); }

   private:
    std::mt19937 gen;
    std::uniform_int_distribution<int> distribution;
};

#endif //INC_RANDOM_GENERATOR_H__
