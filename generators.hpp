#pragma once
#include "util.hpp"
#include <cmath>
#include <functional>
#include <memory>
#include <numbers>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class Generator;

extern std::mt19937_64 __rng_eng;
extern std::vector<std::pair<std::vector<std::uint8_t>,
                             std::unique_ptr<Generator> (*)(const std::vector<std::uint8_t> &)>>
    gen_serializers;
#define DESERIALIZABLE_GEN(class_name)                                                             \
    namespace {                                                                                    \
        [[maybe_unused]] const auto __add_serializable_generator__##class_name                     \
            = gen_serializers.emplace_back(                                                        \
                std::ranges::to<std::vector<std::uint8_t>>(                                        \
                    range_to_bytes(class_name::unique_name)),                                      \
                [](const std::vector<std::uint8_t> &bin_data) -> std::unique_ptr<Generator> {      \
                    return std::make_unique<class_name>(bin_data);                                 \
                });                                                                                \
    }

class Generator {
protected:
    double m_amplitude;
    int m_t_start;
    int m_t_end;

    constexpr static std::size_t dump_size = (sizeof m_amplitude) + (sizeof m_t_start) * 2;
    static_assert(std::is_same_v<decltype(m_t_start), decltype(m_t_end)>);

    constexpr bool enabled_time(int time) const noexcept
    {
        return (m_t_start == 0 && m_t_end == 0) || (time >= m_t_start && time <= m_t_end);
    }
    static constexpr void validate_time(int t_start, int t_end)
    {
        if (t_end < t_start)
            throw std::runtime_error{ "t_end cannot be smaller than t_start" };
    }
    constexpr virtual bool eq(const Generator &b) const
    {
        return m_amplitude == b.m_amplitude && m_t_start == b.m_t_start && m_t_end == b.m_t_end;
    };

public:
    constexpr Generator(double amplitude, int t_start = 0, int t_end = 0)
        : m_amplitude{ amplitude }
        , m_t_start{ t_start }
        , m_t_end{ t_end }
    {
        validate_time(t_start, t_end);
    }
    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    constexpr Generator(const T &serialized)
    {
        using B = std::ranges::range_value_t<T>;
        constexpr auto ampl_size = sizeof m_amplitude;
        constexpr auto time_size = (sizeof m_t_start) * 2;
        static_assert(ampl_size + time_size == dump_size);

        if (std::ranges::size(serialized) < dump_size)
            throw std::runtime_error{ "Not enough data to construct a Generator" };

        std::array<B, ampl_size> amplitude_bytes;
        std::array<B, time_size> time_bytes;
        std::ranges::copy_n(std::ranges::begin(serialized), ampl_size, amplitude_bytes.begin());
        std::ranges::copy_n(std::ranges::drop_view{ serialized, ampl_size }.begin(), time_size,
                            time_bytes.begin());
        const auto [ts, te] = array_from_bytes<int, 2>(time_bytes);
        m_amplitude = from_bytes<double>(amplitude_bytes);
        m_t_start = ts;
        m_t_end = te;
    }
    constexpr double get_amplitude() const noexcept { return m_amplitude; }
    constexpr std::pair<int, int> get_activity_time() const noexcept
    {
        return { m_t_start, m_t_end };
    }
    constexpr void set_amplitude(double amplitude) { m_amplitude = amplitude; }
    constexpr void set_activity_time(int t_start, int t_end)
    {
        validate_time(t_start, t_end);
        m_t_start = t_start;
        m_t_end = t_end;
    }
    virtual double symuluj(int time) = 0;
    virtual constexpr std::vector<uint8_t> dump() const
    {
        const auto amplitude_dump = to_bytes(m_amplitude);
        const auto time = to_bytes(std::array{ m_t_start, m_t_end });
        if consteval {
            static_assert((sizeof amplitude_dump) + (sizeof time) == dump_size);
        }
        return concat_iterables(amplitude_dump, time);
    }
    constexpr virtual ~Generator(){};

    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    static std::unique_ptr<Generator> deserialize(const T &serialized)
    {
        for (const auto &[name, factory] : gen_serializers) {
#if __cpp_lib_ranges_starts_ends_with >= 202106L
            if (std::ranges::starts_with(serialized, name))
#else
            if (std::ranges::mismatch(serialized, name).in2 == std::ranges::end(name))
#endif
                return factory(serialized | std::ranges::to<std::vector<uint8_t>>());
        }
        throw std::runtime_error{ "Serialized data does not match any known generator." };
    }

    friend bool operator==(const Generator &a, const Generator &b)
    {
        return typeid(a) == typeid(b) && a.eq(b);
    }
};

class GeneratorDecor : public Generator {
protected:
    std::unique_ptr<Generator> m_base{};
    virtual double simulate_internal(int time) = 0;
    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorDecor &>(b);
        return Generator::eq(bp) && *m_base == *bp.m_base;
    }

public:
    constexpr GeneratorDecor(std::unique_ptr<Generator> &&base, double amplitude, int t_start = 0,
                             int t_end = 0)
        : Generator{ amplitude, t_start, t_end }
        , m_base{ std::move(base) }
    {
        if (!m_base)
            throw std::runtime_error{ "base must not be null" };
    }
    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    GeneratorDecor(const T &serialized)
        : Generator{ serialized }
    {
        auto remaining = std::ranges::drop_view{ serialized, Generator::dump_size };
        m_base = deserialize(remaining);
    }
    double symuluj(int time) override { return m_base->symuluj(time) + simulate_internal(time); }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(Generator::dump(), m_base->dump());
    }

    // I don't want to make a getter for the pointer, so GeneratorsConfig must be a friend
    friend class GeneratorsConfig;
};

class GeneratorBaza : public Generator {
public:
    static constexpr std::string_view unique_name{ "base" };

    constexpr GeneratorBaza(double value = 0.0, int t_start = 0, int t_end = 0)
        : Generator{ value, t_start, t_end }
    {
    }
    constexpr GeneratorBaza(const std::ranges::input_range auto &serialized)
        : Generator{ std::ranges::drop_view{ serialized, unique_name.size() } }
    {
        if (!prefix_match(unique_name, serialized))
            throw std::runtime_error{
                "GeneratorBaza serialized data does not start with expected prefix"
            };
    }
    constexpr void set_value(double value = 0.0) noexcept { set_amplitude(value); };
    constexpr double symuluj(int time) override { return enabled_time(time) ? m_amplitude : 0.0; }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), Generator::dump());
    }

    friend bool operator==(const GeneratorBaza &a, const GeneratorBaza &b)
    {
        return static_cast<const Generator &>(a) == static_cast<const Generator &>(b);
    }
};
DESERIALIZABLE_GEN(GeneratorBaza);

class GeneratorPeriodic : public GeneratorDecor {
protected:
    uint32_t m_period;

    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorPeriodic &>(b);
        return GeneratorDecor::eq(bp) && m_period == bp.m_period;
    }

public:
    constexpr GeneratorPeriodic(std::unique_ptr<Generator> &&base, double amplitude,
                                uint32_t period, int t_start = 0, int t_end = 0)
        : GeneratorDecor{ std::move(base), amplitude, t_start, t_end }
        , m_period{ period }
    {
    }
    constexpr GeneratorPeriodic(const std::ranges::input_range auto &serialized)
        : GeneratorDecor{ std::ranges::drop_view{ serialized, sizeof m_period } }
    {
        using B = std::ranges::range_value_t<decltype(serialized)>;
        std::array<B, sizeof m_period> period_bytes;
        std::ranges::copy_n(std::ranges::begin(serialized), sizeof m_period, period_bytes.begin());
        m_period = from_bytes<decltype(m_period)>(period_bytes);
    }
    constexpr uint32_t get_period() const noexcept { return m_period; }
    constexpr void set_period(uint32_t period) noexcept { m_period = period; }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(to_bytes(m_period), GeneratorDecor::dump());
    }
};

class GeneratorSinus : public GeneratorPeriodic {
private:
    // std::sin is constexpr since C++26, but this is legal since C++23
    constexpr double simulate_internal(int time) override
    {
        return enabled_time(time)
            // Should (time - m_start_time) be used instead of time? This question applies to all
            // non-constant signals
            ? m_amplitude * std::sin(2.0 * std::numbers::pi * (time % m_period) / m_period)
            : 0.0;
    }

public:
    static constexpr std::string_view unique_name{ "sin" };
    constexpr GeneratorSinus(std::unique_ptr<Generator> &&base, double amplitude, uint32_t period,
                             int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
    {
    }
    constexpr GeneratorSinus(const std::ranges::input_range auto &serialized)
        : GeneratorPeriodic{ std::ranges::drop_view{ serialized, unique_name.size() } }
    {
        if (!prefix_match(unique_name, serialized))
            throw std::runtime_error{
                "GeneratorSinus serialized data does not start with expected prefix"
            };
    }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), GeneratorPeriodic::dump());
    }

    friend bool operator==(const GeneratorSinus &a, const GeneratorSinus &b) { return a.eq(b); }
};
DESERIALIZABLE_GEN(GeneratorSinus);

class GeneratorProstokat : public GeneratorPeriodic {
private:
    double m_duty_cycle;

    static constexpr void validate_duty_cycle(double duty_cycle)
    {
        if (duty_cycle >= 1.0 || duty_cycle <= 0.0 || !std::isfinite(duty_cycle))
            throw std::runtime_error{ "Duty cycle should be between 0 and 1. If you want a "
                                      "constant signal use GeneratorBaza." };
    }
    constexpr double simulate_internal(int time) override
    {
        return enabled_time(time) && (time % m_period) < m_duty_cycle * m_period ? m_amplitude
                                                                                 : 0.0;
    }
    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorProstokat &>(b);
        return GeneratorPeriodic::eq(bp) && m_duty_cycle == bp.m_duty_cycle;
    }

public:
    static constexpr std::string_view unique_name{ "pwm" };
    constexpr GeneratorProstokat(std::unique_ptr<Generator> &&base, double amplitude,
                                 uint32_t period, double duty_cycle, int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
        , m_duty_cycle{ duty_cycle }
    {
        validate_duty_cycle(duty_cycle);
    }
    constexpr GeneratorProstokat(const std::ranges::input_range auto &serialized)
        : GeneratorPeriodic{ std::ranges::drop_view{ serialized,
                                                     unique_name.size() + (sizeof m_duty_cycle) } }
    {
        if (!prefix_match(unique_name, serialized))
            throw std::runtime_error{
                "GeneratorProstokat serialized data does not start with expected prefix"
            };

        using B = std::ranges::range_value_t<decltype(serialized)>;
        std::array<B, sizeof m_duty_cycle> duty_cycle_bytes;
        std::ranges::drop_view remaining{ serialized, unique_name.size() };
        std::ranges::copy_n(std::ranges::begin(remaining), sizeof m_duty_cycle,
                            duty_cycle_bytes.begin());
        m_duty_cycle = from_bytes<decltype(m_duty_cycle)>(duty_cycle_bytes);
    }
    constexpr double get_duty_cycle() const noexcept { return m_duty_cycle; }
    constexpr void set_duty_cycle(double duty_cycle)
    {
        validate_duty_cycle(duty_cycle);
        m_duty_cycle = duty_cycle;
    }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), to_bytes(m_duty_cycle),
                                GeneratorPeriodic::dump());
    }

    friend bool operator==(const GeneratorProstokat &a, const GeneratorProstokat &b)
    {
        return a.eq(b);
    }
};
DESERIALIZABLE_GEN(GeneratorProstokat);

class GeneratorSawtooth : public GeneratorPeriodic {
private:
    constexpr double simulate_internal(int time) override
    {
        return enabled_time(time)
            ? m_amplitude * (2 * static_cast<double>(time % m_period) / m_period - 1)
            : 0.0;
    }

public:
    static constexpr std::string_view unique_name{ "saw" };
    constexpr GeneratorSawtooth(std::unique_ptr<Generator> &&base, double amplitude,
                                uint32_t period, int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
    {
    }
    constexpr GeneratorSawtooth(const std::ranges::input_range auto &serialized)
        : GeneratorPeriodic{ std::ranges::drop_view{ serialized, unique_name.size() } }
    {
        if (!prefix_match(unique_name, serialized))
            throw std::runtime_error{
                "GeneratorSawtooth serialized data does not start with expected prefix"
            };
    }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), GeneratorPeriodic::dump());
    }

    friend bool operator==(const GeneratorSawtooth &a, const GeneratorSawtooth &b)
    {
        return a.eq(b);
    }
};
DESERIALIZABLE_GEN(GeneratorSawtooth);

template <typename D> class GeneratorRandomBase : public GeneratorDecor {
protected:
    D m_dist{};

public:
    GeneratorRandomBase(std::unique_ptr<Generator> &&base, double amplitude, int t_start = 0,
                        int t_end = 0)
        : GeneratorDecor{ std::move(base), amplitude, t_start, t_end }
    {
    }
    GeneratorRandomBase(const std::ranges::input_range auto &serialized)
        : GeneratorDecor{ serialized }
    {
    }
};

class GeneratorUniformNoise : public GeneratorRandomBase<std::uniform_real_distribution<>> {
private:
    double simulate_internal(int) override
    {
        return 2.0 * this->m_amplitude * (this->m_dist(__rng_eng) - 0.5);
    }

public:
    static constexpr std::string_view unique_name{ "rand_uniform" };
    GeneratorUniformNoise(std::unique_ptr<Generator> &&base, double amplitude, int t_start = 0,
                          int t_end = 0)
        : GeneratorRandomBase<std::uniform_real_distribution<>>{ std::move(base), amplitude,
                                                                 t_start, t_end }
    {
    }
    GeneratorUniformNoise(const std::ranges::input_range auto &serialized)
        : GeneratorRandomBase<std::uniform_real_distribution<>>{ std::ranges::drop_view{
              serialized, unique_name.size() } }
    {
        if (!prefix_match(unique_name, serialized))
            throw std::runtime_error{
                "GeneratorUniformNoise serialized data does not start with expected prefix"
            };
    }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), GeneratorDecor::dump());
    }

    friend bool operator==(const GeneratorUniformNoise &a, const GeneratorUniformNoise &b)
    {
        return a.eq(b);
    }
};
DESERIALIZABLE_GEN(GeneratorUniformNoise);

class GeneratorNormalNoise : public GeneratorRandomBase<std::normal_distribution<>> {
private:
    using pt = std::normal_distribution<>::param_type;

    double m_stddev;

    double simulate_internal(int) override
    {
        return this->m_dist(__rng_eng, pt{ this->m_amplitude, m_stddev });
    }
    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorNormalNoise &>(b);
        return GeneratorRandomBase<std::normal_distribution<>>::eq(bp) && m_stddev == bp.m_stddev;
    }

public:
    static constexpr std::string_view unique_name{ "rand_normal" };
    GeneratorNormalNoise(std::unique_ptr<Generator> &&base, double mean, double stddev,
                         int t_start = 0, int t_end = 0)
        : GeneratorRandomBase<std::normal_distribution<>>{ std::move(base), mean, t_start, t_end }
        , m_stddev{ stddev }
    {
    }
    GeneratorNormalNoise(const std::ranges::input_range auto &serialized)
        : GeneratorRandomBase<std::normal_distribution<>>{ std::ranges::drop_view{
              serialized, unique_name.size() + sizeof m_stddev } }
    {
        if (!prefix_match(unique_name, serialized))
            throw std::runtime_error{
                "GeneratorNormalNoise serialized data does not start with expected prefix"
            };

        using B = std::ranges::range_value_t<decltype(serialized)>;
        std::array<B, sizeof m_stddev> stddev_bytes;
        std::ranges::drop_view remaining{ serialized, unique_name.size() };
        std::ranges::copy_n(std::ranges::begin(remaining), sizeof m_stddev, stddev_bytes.begin());
        m_stddev = from_bytes<decltype(m_stddev)>(stddev_bytes);
    }
    constexpr double get_mean() const noexcept { return get_amplitude(); }
    constexpr void set_mean(double mean) { this->set_amplitude(mean); }
    constexpr double get_stddev() const noexcept { return m_stddev; }
    constexpr void set_stddev(double stddev) { m_stddev = stddev; }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), to_bytes(m_stddev),
                                GeneratorDecor::dump());
    }

    friend bool operator==(const GeneratorNormalNoise &a, const GeneratorNormalNoise &b)
    {
        return a.eq(b);
    }
};
DESERIALIZABLE_GEN(GeneratorNormalNoise);

#ifdef LAB_TESTS
class GeneratorTests {
    static auto get_base();
    static void test_base();
    static void test_activity_time();
    static void test_sin();
    static void test_rect();
    static void test_sawtooth();
    static void test_addition();
    static void test_serialization();

public:
    static void run_tests();
};
#endif
