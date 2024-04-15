#pragma once
#include <concepts>

template <std::floating_point T = double> class ObiektSISO {
public:
    virtual T symuluj(T u) = 0;
    virtual ~ObiektSISO() = default;
};
