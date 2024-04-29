#pragma once
#include "../lab1/ObiektSISO.h"
#include "../lab1/util.hpp"
#include <iostream>
#include <iterator>
#include <type_traits>
#include <vector>

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
    constexpr RegulatorPID(const std::vector<uint8_t> &data)
        : RegulatorPID{ data.cbegin(), data.cend() } {};
    template <typename Iter>
        requires std::contiguous_iterator<Iter>
        && std::is_same_v<typename std::iterator_traits<Iter>::value_type, uint8_t>
    constexpr RegulatorPID(Iter start, Iter end)
    {
        constexpr std::size_t expected_size = 40UL;
        const auto data_size{ std::distance(start, end) };
        if (data_size < static_cast<std::ptrdiff_t>(expected_size))
            throw std::runtime_error{ "Data size is smaller than expected" };

        std::array<uint8_t, expected_size> byte_array;
        std::copy_n(start, expected_size, byte_array.begin());
        const auto [k, ti, td, integral, prev_e] = array_from_bytes<double, 5UL>(byte_array);
        m_k = k;
        m_ti = ti;
        m_td = td;
        m_integral = integral;
        m_prev_e = prev_e;
        check_constraints();
    }
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
    }
    constexpr std::vector<uint8_t> dump() const
    {
        auto bytes = to_bytes(std::array{ m_k, m_ti, m_td, m_integral, m_prev_e });
        return std::vector(bytes.begin(), bytes.end());
    }

    friend bool operator==(const RegulatorPID &, const RegulatorPID &) = default;
    friend bool operator!=(const RegulatorPID &, const RegulatorPID &) = default;
    friend std::ostream &operator<<(std::ostream &os, const RegulatorPID &m);
    friend std::istream &operator>>(std::istream &is, RegulatorPID &m);
};

class Testy_RegulatorPID {
private:
    static void test_RegulatorP_brakPobudzenia();
    static void test_RegulatorP_skokJednostkowy();
    static void test_RegulatorPI_skokJednostkowy_1();
    static void test_RegulatorPI_skokJednostkowy_2();
    static void test_RegulatorPID_skokJednostkowy();
    constexpr static RegulatorPID get_test_model();
    static void test_dump_eq();
    static void test_dump_length();
    static void test_dump_file();
    static void test_stream_op();

public:
    static void run_tests();
};
