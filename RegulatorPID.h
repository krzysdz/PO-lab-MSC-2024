#pragma once
#include "ObiektSISO.h"
#include "util.hpp"
#include <iostream>
#include <iterator>
#include <type_traits>
#include <vector>

/// PID regulator implementation derived from ObiektSISO.
class RegulatorPID : public ObiektSISO {
public:
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "rPID" };

private:
    /// Size of the unique prefix.
    static constexpr std::size_t prefix_size{ unique_name.size()
                                              * sizeof(decltype(unique_name)::value_type) };
    /// Proportional part multiplier (gain).
    double m_k;
    /// Integration constant.
    double m_ti;
    /// Differentiation constant.
    double m_td;
    /// Integration accumulator.
    double m_integral{};
    /// Previous input.
    double m_prev_e{};

    /// @brief Check if constraints regarding parameter values are met.
    /// @throws `std::runtime_error` if is_bad_or_neg() is `true` for #m_k, #m_ti or #m_td.
    constexpr void check_constraints() const
    {
        if (is_bad_or_neg(m_k) || is_bad_or_neg(m_ti) || is_bad_or_neg(m_td))
            throw std::runtime_error{ "PID parameters must be nonnegative finite numbers" };
    };
    /// @brief Proportional part of PID simulation
    /// @param e input to the regulator
    constexpr double sim_propoprtional(const double e) const { return m_k * e; }
    /// @brief Integral part of PID simulation
    /// @param e input to the regulator
    /// @return `0.0` if #m_ti is `0.0`, otherwise the result of the I part of PID
    constexpr double sim_integral(const double e)
    {
        if (m_ti > 0.0) {
            m_integral += e / m_ti;
            return m_integral;
        }
        return 0.0;
    }
    /// @brief Derivative part of PID simulation
    /// @param e input to the regulator
    constexpr double sim_derviative(const double e)
    {
        const auto diff{ e - m_prev_e };
        m_prev_e = e;
        return m_td * diff;
    }

public:
    /// @brief Deserializing constructor from vector of `uint8_t`.
    ///
    /// Uses @link RegulatorPID::RegulatorPID(Iter, Iter)@endlink with the `data` vector's
    /// iterators.
    ///
    /// @param data vector of bytes representing serialized RegulatorPID
    constexpr RegulatorPID(const std::vector<uint8_t> &data)
        : RegulatorPID{ data.cbegin(), data.cend() } {};
    /// @brief Deserializing constructor from a pair of iterators over `uint8_t`.
    /// @param start iterator to the beginning of the range
    /// @param end end iterator of the range
    template <typename Iter>
        requires std::contiguous_iterator<Iter>
        && std::is_same_v<typename std::iterator_traits<Iter>::value_type, uint8_t>
    constexpr RegulatorPID(Iter start, Iter end)
    {
        constexpr std::size_t len_size = sizeof(uint32_t);
        constexpr std::size_t expected_data_size = 40UL;
        constexpr std::size_t expected_total_size = expected_data_size + prefix_size + len_size;
        const auto data_size{ std::distance(start, end) };
        if (data_size < static_cast<std::ptrdiff_t>(expected_total_size))
            throw std::runtime_error{ "Data size is smaller than expected" };

        if (!prefix_match(unique_name, std::ranges::subrange{ start + len_size, end }))
            throw std::runtime_error{
                "RegulatorPID serialized data does not start with the expected prefix"
            };

        std::array<uint8_t, expected_data_size> byte_array;
        std::copy_n(start + prefix_size + len_size, expected_data_size, byte_array.begin());
        const auto [k, ti, td, integral, prev_e] = array_from_bytes<double, 5UL>(byte_array);
        m_k = k;
        m_ti = ti;
        m_td = td;
        m_integral = integral;
        m_prev_e = prev_e;
        check_constraints();
    }
    /// @brief Regular constructor accepting basic PID parameters.
    /// @param k proportional gain, must be >= 0
    /// @param ti integration constant, must be >= 0, 0 means disabled
    /// @param td derivation constant, must be >= 0
    constexpr RegulatorPID(const double k, const double ti = 0.0, const double td = 0.0)
        : m_k{ k }
        , m_ti{ ti }
        , m_td{ td }
    {
        check_constraints();
    }
    /// Gain (#m_k) getter.
    constexpr double get_k() const noexcept { return m_k; }
    /// Integration constant (#m_ti) getter.
    constexpr double get_ti() const noexcept { return m_ti; }
    /// Derivation constant (#m_td) getter.
    constexpr double get_td() const noexcept { return m_td; }
    /// @brief Gain (#m_k) setter.
    /// @param k new value, must be >= 0
    constexpr void set_k(const double k)
    {
        throw_bad_neg(k);
        m_k = k;
    }
    /// @brief Integration constant (#m_ti) setter.
    /// @param ti new value, must be >= 0
    constexpr void set_ti(const double ti)
    {
        throw_bad_neg(ti);
        m_ti = ti;
    }
    /// @brief Derivation constant (#m_td) setter.
    /// @param td new value, must be >= 0
    constexpr void set_td(const double td)
    {
        throw_bad_neg(td);
        m_td = td;
    }
    /// @brief Simulate PID response given input `e`.
    ///
    /// The PID simulation is described by the following formula:
    ///
    /// @f[
    ///   u_i = k \times e_i + \frac{1}{T_I} \sum_{j=0}^i e_j + T_D(e_i - e_{i-1})
    /// @f]
    ///
    /// Where:
    /// - @f$u_i@f$ is the output
    /// - @f$e_i@f$ is the input
    /// - @f$e_{i-1}@f$ is the previous input (#m_prev_e)
    /// - @f$k@f$ is #m_k
    /// - @f$T_I@f$ is #m_ti
    /// - @f$T_D@f$ is #m_td
    ///
    /// If #m_ti is 0, the integral part (with sum and @f$\frac{1}{T_I}@f$) is not simulated
    ///
    /// @param e input to the regulator
    /// @return PID regulator simulation result.
    constexpr double symuluj(const double e) override
    {
        return sim_propoprtional(e) + sim_integral(e) + sim_derviative(e);
    }
    constexpr std::vector<uint8_t> dump() const override
    {
        auto bytes = to_bytes(std::array{ m_k, m_ti, m_td, m_integral, m_prev_e });
        constexpr auto dump_size_b = to_bytes(static_cast<uint32_t>(prefix_size + bytes.size()));
        return concat_iterables(dump_size_b, range_to_bytes(unique_name), bytes);
    }
    /// Reset state of the regulator by setting #m_integral and #m_prev_e to 0.
    constexpr void reset() noexcept override
    {
        m_integral = 0.0;
        m_prev_e = 0.0;
    }

    friend bool operator==(const RegulatorPID &, const RegulatorPID &) = default;
    friend bool operator!=(const RegulatorPID &, const RegulatorPID &) = default;
    /// Stream output operator, which writes full object state in text form.
    friend std::ostream &operator<<(std::ostream &os, const RegulatorPID &m);
    /// Stream input operator, which can reconfigure the object to match provided the text form.
    friend std::istream &operator>>(std::istream &is, RegulatorPID &m);
};
DESERIALIZABLE_SISO(RegulatorPID);

#ifdef LAB_TESTS
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
#endif
