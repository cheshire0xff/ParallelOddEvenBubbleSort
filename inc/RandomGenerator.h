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
