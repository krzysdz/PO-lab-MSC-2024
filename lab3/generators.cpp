#include "generators.hpp"

std::mt19937_64 __rng_eng{ std::random_device{}() };
std::vector<std::pair<std::vector<std::uint8_t>,
                      std::unique_ptr<Generator> (*)(const std::vector<std::uint8_t> &)>>
    gen_serializers;
