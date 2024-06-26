#pragma once
#include <cstdint>
#include <cstring>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>
#include <version>
#if __cpp_lib_format >= 201907L
#include <format>
#endif

#include "ObiektSISO.h"

class ModelARX : public ObiektSISO {
public:
    static constexpr std::string_view unique_name{ "mARX" };

private:
    static constexpr std::size_t prefix_size{ unique_name.size()
                                              * sizeof(decltype(unique_name)::value_type) };

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
    ModelARX(Iter start, Iter end)
    {
        const auto data_size{ std::distance(start, end) };
        if (data_size
            < static_cast<std::ptrdiff_t>(sizeof(raw_data_t) + prefix_size + sizeof(uint32_t)))
            throw std::runtime_error{ "Data size is smaller than constant-length part" };

        // skip the length
        start += sizeof(uint32_t);
        const uint8_t *s_ptr = &*start;
        if (!prefix_match(unique_name, std::ranges::subrange{ start, end }))
            throw std::runtime_error{
                "ModelARX serialized data does not start with the expected prefix"
            };

        s_ptr += prefix_size;
        raw_data_t raw_data;
        std::memcpy(&raw_data, s_ptr, sizeof(raw_data_t));
        const auto expected_size{ static_cast<std::ptrdiff_t>(
            (raw_data.n_coeff_a + raw_data.n_coeff_b + raw_data.in_n + raw_data.out_n
             + raw_data.delay_n)
                * 8
            + sizeof(raw_data_t) + prefix_size + sizeof(uint32_t)) };
        if (data_size != expected_size)
            throw std::runtime_error{
#if __cpp_lib_format >= 201907L
                std::format("Data size ({} bytes) does not match the expected size ({} bytes)",
                            data_size, expected_size)
#else
                "Data size does not match the expected size"
#endif
            };

        auto vec_start = reinterpret_cast<const double *>(s_ptr + sizeof(raw_data_t));
        auto vec_end = vec_start + raw_data.n_coeff_a;
        m_coeff_a = std::vector<double>(vec_start, vec_end);
        vec_start = vec_end;
        vec_end += raw_data.n_coeff_b;
        m_coeff_b = std::vector<double>(vec_start, vec_end);
        vec_start = vec_end;
        vec_end += raw_data.in_n;
        m_in_signal_mem = decltype(m_in_signal_mem)(vec_start, vec_end);
        vec_start = vec_end;
        vec_end += raw_data.out_n;
        m_out_signal_mem = decltype(m_out_signal_mem)(vec_start, vec_end);
        vec_start = vec_end;
        vec_end += raw_data.delay_n;
        m_delay_mem = decltype(m_delay_mem)(vec_start, vec_end);

        m_transport_delay = static_cast<uint32_t>(raw_data.delay_n);
        m_init_seed = raw_data.init_seed;
        // Restore generator state by initializing it with original seed and skipping the same
        // number of results. It may be inefficient, but the STL implementation of Mersenne Twister
        // does not expose the internal state and I don't want to use implementation-specific
        // solutions, which could break in other versions of libstdc++, libc++ and MSVC STL.
        // m_mt.discard(m_n_generated) won't work, because the distribution has its own state...
        m_mt = std::mt19937_64{ static_cast<decltype(m_mt)::result_type>(m_init_seed) };
        m_distribution = decltype(m_distribution)(raw_data.dist_mean, raw_data.dist_stddev);
        while (m_n_generated < raw_data.n_generated)
            get_random();
    }
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
    std::vector<uint8_t> dump() const override;
    void reset() override;

    friend bool operator==(const ModelARX &, const ModelARX &) = default;
    friend bool operator!=(const ModelARX &, const ModelARX &) = default;
    friend std::ostream &operator<<(std::ostream &os, const ModelARX &m);
    friend std::istream &operator>>(std::istream &is, ModelARX &m);
#ifndef NO_LAB_TESTS
    friend class Testy_ModelARX;
#endif
};
DESERIALIZABLE_SISO(ModelARX);

#ifndef NO_LAB_TESTS
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
#endif
