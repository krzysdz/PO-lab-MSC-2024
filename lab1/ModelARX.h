#pragma once
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include "ObiektSISO.h"

class ModelARX : public ObiektSISO {
private:
    std::vector<double> m_coeff_a;
    std::vector<double> m_coeff_b;
    uint32_t m_transport_delay;
    std::normal_distribution<double> m_distribution;
    std::deque<double> m_in_signal_mem;
    std::deque<double> m_out_signal_mem;
    std::deque<double> m_delay_mem;
    std::uint64_t m_init_seed;
    std::uint64_t m_n_generated{};
    std::mt19937_64 m_mt;

    struct raw_data_t {
        uint64_t n_coeff_a;
        uint64_t n_coeff_b;
        double dist_mean;
        double dist_stddev;
        uint64_t in_n;
        uint64_t out_n;
        uint64_t delay_n;
        uint64_t init_seed;
        uint64_t n_generated;
    };
    static_assert(std::is_trivial_v<raw_data_t> && std::is_standard_layout_v<raw_data_t>);

    double get_random();

public:
    ModelARX() = delete;
    ModelARX(const std::vector<uint8_t> &data)
        : ModelARX{ data.cbegin(), data.cend() } {};
    template <typename Iter>
        requires std::contiguous_iterator<Iter>
        && std::is_same_v<typename std::iterator_traits<Iter>::value_type, uint8_t>
    ModelARX(Iter start, Iter end);
    ModelARX(std::vector<double> &&coeff_a, std::vector<double> &&coeff_b, const int32_t delay = 1,
             const double stddev = 0.0);
    void set_coeff_a(std::vector<double> &&coefficients) noexcept;
    void set_coeff_b(std::vector<double> &&coefficients) noexcept;
    void set_transport_delay(const int32_t delay);
    void set_stddev(const double stddev);
    double symuluj(double u) override;
    /// @brief Prepare a binary dump of the model. Format is platform-specific, for sure won't work
    /// with different endianness
    /// @return Byte buffer containing all data necessary to restore the object
    std::vector<uint8_t> dump() const;

    friend bool operator==(const ModelARX &, const ModelARX &) = default;
    friend bool operator!=(const ModelARX &, const ModelARX &) = default;
    friend std::ostream &operator<<(std::ostream &os, const ModelARX &m);
    friend std::istream &operator>>(std::istream &is, ModelARX &m);
    friend class Testy_ModelARX;
};

class Testy_ModelARX {
private:
    static void test_ModelARX_brakPobudzenia();
    static void test_ModelARX_skokJednostkowy_1();
    static void test_ModelARX_skokJednostkowy_2();
    static void test_ModelARX_skokJednostkowy_3();
    static ModelARX get_test_model();
    static void test_dump_eq();
    static void test_dump_length();
    static void test_dump_very_small();
    static void test_dump_file();
    static void test_stream_op();

public:
    static void run_tests();
};
