#include "RegulatorPID.h"
#include "../lab1/util.hpp"
#include <cmath>
#include <stdexcept>

constexpr void RegulatorPID::check_constraints() const
{
    if (is_bad_or_neg(m_k) || is_bad_or_neg(m_ti) || is_bad_or_neg(m_td))
        throw std::runtime_error{ "PID parameters must be nonnegative finite numbers" };
}

constexpr double RegulatorPID::symuluj(double u) { return 0.0; }
