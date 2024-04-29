#pragma once
#include "../lab1/ObiektSISO.h"
#include "../lab1/util.hpp"
#include <iostream>

class RegulatorPID : public ObiektSISO {
private:
    double m_k;
    double m_ti;
    double m_td;
    double m_integral{};
    double m_prev_e{};

    constexpr void check_constraints() const
    {
        if (is_bad_or_neg(m_k) || is_bad_or_neg(m_ti) || is_bad_or_neg(m_td))
            throw std::runtime_error{ "PID parameters must be nonnegative finite numbers" };
    };
    constexpr double sim_propoprtional(const double e) const { return m_k * e; }
    constexpr double sim_integral(const double e)
    {
        if (m_ti > 0.0) {
            m_integral += e / m_ti;
            return m_integral;
        }
        return 0.0;
    }
    constexpr double sim_derviative(const double e)
    {
        const auto diff{ e - m_prev_e };
        m_prev_e = e;
        return m_td * diff;
    }

public:
    constexpr RegulatorPID(const double k, const double ti = 0.0, const double td = 0.0)
        : m_k{ k }
        , m_ti{ ti }
        , m_td{ td }
    {
        check_constraints();
    }
    constexpr void set_k(const double k)
    {
        throw_bad_neg(k);
        m_k = k;
    }
    constexpr void set_ti(const double ti)
    {
        throw_bad_neg(ti);
        m_ti = ti;
    }
    constexpr void set_td(const double td)
    {
        throw_bad_neg(td);
        m_td = td;
    }
    constexpr double symuluj(const double e) override
    {
        return sim_propoprtional(e) + sim_integral(e) + sim_derviative(e);
    };

    friend bool operator==(const RegulatorPID &, const RegulatorPID &) = default;
    friend bool operator!=(const RegulatorPID &, const RegulatorPID &) = default;
    friend std::ostream &operator<<(std::ostream &os, const RegulatorPID &m);
    friend std::istream &operator>>(std::istream &is, RegulatorPID &m);
};
