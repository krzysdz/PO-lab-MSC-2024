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

/// Autoregressive exogenous model implementation derived from ObiektSISO.
class ModelARX : public ObiektSISO {
public:
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "mARX" };

private:
    /// Size of the unique prefix.
    static constexpr std::size_t prefix_size{ unique_name.size()
                                              * sizeof(decltype(unique_name)::value_type) };

    /// Polynomial A coefficients.
    std::vector<double> m_coeff_a;
    /// Polynomial B coefficients.
    std::vector<double> m_coeff_b;
    /// Delay of input samples.
    uint32_t m_transport_delay;
    /// Distribution used for noise generation.
    std::normal_distribution<double> m_distribution;
    /// Queue of input samples after delay.
    std::deque<double> m_in_signal_mem;
    /// Queue of output samples.
    std::deque<double> m_out_signal_mem;
    /// Queue of input samples being delayed.
    std::deque<double> m_delay_mem;
    /// Initial seed of random number generator.
    std::uint64_t m_init_seed;
    /// Number of random numbers generated since seeding the generator.
    std::uint64_t m_n_generated{};
    /// Mersenne Twister engine used as PRNG.
    std::mt19937_64 m_mt;

    /// Helper structure containing class properties with known size for easier (de)serialization.
    struct raw_data_t {
        /// Size of ModelARX::m_coeff_a.
        uint64_t n_coeff_a;
        /// Size of ModelARX::m_coeff_b.
        uint64_t n_coeff_b;
        /// Mean of ModelARX::m_distribution.
        double dist_mean;
        /// Standard deviation of ModelARX::m_distribution.
        double dist_stddev;
        /// Size of ModelARX::m_in_signal_mem.
        uint64_t in_n;
        /// Size of ModelARX::m_out_signal_mem.
        uint64_t out_n;
        /// Size of ModelARX::m_delay_mem (equal to ModelARX::m_transport_delay).
        uint64_t delay_n;
        /// Copy of ModelARX::m_init_seed.
        uint64_t init_seed;
        /// Copy of ModelARX::m_n_generated.
        uint64_t n_generated;
    };
    static_assert(std::is_trivial_v<raw_data_t> && std::is_standard_layout_v<raw_data_t>);

    /// Draw a random number from #m_mt using #m_distribution and increment #m_n_generated counter.
    double get_random();

public:
    ModelARX() = delete;
    /// @brief Deserializing constructor from vector of `uint8_t`.
    ///
    /// Uses @link ModelARX::ModelARX(Iter, Iter)@endlink with the `data` vector's iterators.
    ///
    /// @param data vector of bytes representing serialized ModelARX
    ModelARX(const std::vector<uint8_t> &data)
        : ModelARX{ data.cbegin(), data.cend() } {};
    /// @brief Deserializing constructor from a pair of iterators over `uint8_t`.
    /// @param start iterator to the beginning of the range
    /// @param end end iterator of the range
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
    /// @brief Regular constructor accepting basic parameters.
    /// @param coeff_a coefficients of the A polynomial
    /// @param coeff_b coefficients of the B polynomial
    /// @param delay input sample delay
    /// @param stddev standard deviation of noise generator
    ModelARX(std::vector<double> &&coeff_a, std::vector<double> &&coeff_b, const int32_t delay = 1,
             const double stddev = 0.0);
    /// Polynomial A coefficents (#m_coeff_a) getter
    constexpr const std::vector<double> &get_coeff_a() const noexcept { return m_coeff_a; }
    /// Polynomial B coefficents (#m_coeff_b) getter
    constexpr const std::vector<double> &get_coeff_b() const noexcept { return m_coeff_b; }
    /// Transport delay (#m_transport_delay) getter
    constexpr uint32_t get_transport_delay() const noexcept { return m_transport_delay; }
    /// Noise standard deviation getter
    double get_stddev() const { return m_distribution.stddev(); }
    /// Polynomial A coefficents (#m_coeff_a) setter
    void set_coeff_a(std::vector<double> &&coefficients) noexcept;
    /// Polynomial B coefficents (#m_coeff_b) setter
    void set_coeff_b(std::vector<double> &&coefficients) noexcept;
    /// Transport delay (#m_transport_delay) setter
    void set_transport_delay(const int32_t delay);
    /// Noise standard deviation setter
    void set_stddev(const double stddev);
    /// @brief Simulate model's response to input `u`.
    /// @param u input
    /// @return Simulated model's response
    double symuluj(double u) override;
    /// @brief Prepare a binary dump of the model. Format is platform-specific, for sure won't work
    /// with different endianness
    /// @return Byte buffer containing all data necessary to restore the object
    std::vector<uint8_t> dump() const override;
    /// @brief Reset model's state
    ///
    /// Fills all queues with `0`s, reseeds the PRNG, resets distribution state (#m_distribution)
    /// and zeros RNG counter (#m_n_generated).
    void reset() override;

    friend bool operator==(const ModelARX &, const ModelARX &) = default;
    friend bool operator!=(const ModelARX &, const ModelARX &) = default;
    /// Stream output operator, which writes full object state in text form.
    friend std::ostream &operator<<(std::ostream &os, const ModelARX &m);
    /// Stream input operator, which can reconfigure the object to match provided the text form.
    friend std::istream &operator>>(std::istream &is, ModelARX &m);
#ifdef LAB_TESTS
    friend class Testy_ModelARX;
#endif
};
DESERIALIZABLE_SISO(ModelARX);

#ifdef LAB_TESTS
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
