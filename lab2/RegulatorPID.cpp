#include "RegulatorPID.h"
#include <iomanip>

std::ostream &operator<<(std::ostream &os, const RegulatorPID &m)
{
    const auto flags = os.flags();
    const auto precision = os.precision();
    const auto fill = os.fill();
    os.flags(std::ios::dec | std::ios::left | std::ios::fixed);
    os.precision(std::numeric_limits<double>::max_digits10);
    os.fill(' ');
    os << m.m_k << ' ' << m.m_ti << ' ' << m.m_td << ' ' << m.m_integral << ' ' << m.m_prev_e;
    os.fill(fill);
    os.precision(precision);
    os.flags(flags);
    return os << '\n';
}

std::istream &operator>>(std::istream &is, RegulatorPID &m)
{
    double k, ti, td, integral, prev_e;
    const auto flags = is.flags();
    is.flags(std::ios::dec | std::ios::skipws);
    is >> k >> ti >> td >> integral >> prev_e;
    m.set_k(k);
    m.set_ti(ti);
    m.set_td(td);
    m.m_integral = integral;
    m.m_prev_e = prev_e;
    is.flags(flags);
    return is;
}
