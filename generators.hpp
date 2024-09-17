/// @file generators.hpp

#pragma once
#include "util.hpp"
#include <cmath>
#include <format>
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

/// Shared pseudo-global PRNG used by generators.
extern std::mt19937_64 __rng_eng;
/// Vector of [prefix, deserializer function] pairs.
extern std::vector<std::pair<std::vector<std::uint8_t>,
                             std::unique_ptr<Generator> (*)(const std::vector<std::uint8_t> &)>>
    gen_deserializers;
/// @brief Declare class @a class_name as deserializable and add its deserializer to
/// #gen_deserializers.
/// @details The class must have a `::unique_name` member.
#define DESERIALIZABLE_GEN(class_name)                                                             \
    namespace {                                                                                    \
        [[maybe_unused]] const auto __add_serializable_generator__##class_name                     \
            = gen_deserializers.emplace_back(                                                      \
                std::ranges::to<std::vector<std::uint8_t>>(                                        \
                    range_to_bytes(class_name::unique_name)),                                      \
                [](const std::vector<std::uint8_t> &bin_data) -> std::unique_ptr<Generator> {      \
                    return std::make_unique<class_name>(bin_data);                                 \
                });                                                                                \
    }

/// Abstract base class representing a signal generator.
class Generator {
protected:
    /// Amplitude of the signal
    double m_amplitude;
    /// Time at which the generator should be enabled
    int m_t_start;
    /// The last moment at which the generator should be enabled
    int m_t_end;

    /// Size of the serialized base Generator class
    constexpr static std::size_t dump_size = (sizeof m_amplitude) + (sizeof m_t_start) * 2;
    static_assert(std::is_same_v<decltype(m_t_start), decltype(m_t_end)>);

    /// @brief Check whether the generator should be enabled at the given moment.
    /// @param time simulation time at which the test is performed
    constexpr bool enabled_time(int time) const noexcept
    {
        return (m_t_start == 0 && m_t_end == 0) || (time >= m_t_start && time <= m_t_end);
    }
    /// @brief Validate whether end time is not smaller than start time.
    /// @param t_start start time
    /// @param t_end end time
    /// @throws std::runtime_error if `t_end < t_start`
    static constexpr void validate_time(int t_start, int t_end)
    {
        if (t_end < t_start)
            throw std::runtime_error{ "t_end cannot be smaller than t_start" };
    }
    /// @brief Equality check with other generators.
    ///
    /// This method compares only the Generator member variables. Derived classes with additional
    /// member variables must override this method.<br>
    /// The overrides should use the #eq method of their predecessors
    ///
    /// @param b the other generator
    /// @return `true` if the generator has the same properties
    constexpr virtual bool eq(const Generator &b) const
    {
        return m_amplitude == b.m_amplitude && m_t_start == b.m_t_start && m_t_end == b.m_t_end;
    };

public:
    /// @brief Regular Generator constructor accepting basic parameters.
    /// @details If both `t_start` and `t_end` are 0, the generator is always active.
    /// @param amplitude amplitude of the generated signal
    /// @param t_start start time of activity
    /// @param t_end end time of activity (inclusive)
    constexpr Generator(double amplitude, int t_start = 0, int t_end = 0)
        : m_amplitude{ amplitude }
        , m_t_start{ t_start }
        , m_t_end{ t_end }
    {
        validate_time(t_start, t_end);
    }
    /// @brief Deserializing constructor from a range of bytes
    /// @tparam T type of input range over bytes
    /// @param serialized input range over bytes representing serialized Generator
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
    /// Amplitude (#m_amplitude) getter.
    constexpr double get_amplitude() const noexcept { return m_amplitude; }
    /// @brief Activity time getter.
    /// @return <start time, end time> pair
    constexpr std::pair<int, int> get_activity_time() const noexcept
    {
        return { m_t_start, m_t_end };
    }
    /// Amplitude (#m_amplitude) setter.
    constexpr void set_amplitude(double amplitude) { m_amplitude = amplitude; }
    /// @brief Activity time setter.
    /// @param t_start start time
    /// @param t_end end time
    constexpr void set_activity_time(int t_start, int t_end)
    {
        validate_time(t_start, t_end);
        m_t_start = t_start;
        m_t_end = t_end;
    }
    /// @brief Perform simulation of output at given simulation time
    /// @param time simulation time
    /// @return Generator's simulation output.
    virtual double symuluj(int time) = 0;
    /// @brief Serialize the object.
    /// @return A vector of bytes (`uint8_t`) from which the object can be reconstructed.
    virtual constexpr std::vector<uint8_t> dump() const
    {
        const auto amplitude_dump = to_bytes(m_amplitude);
        const auto time = to_bytes(std::array{ m_t_start, m_t_end });
        if consteval {
            static_assert((sizeof amplitude_dump) + (sizeof time) == dump_size);
        }
        return concat_iterables(amplitude_dump, time);
    }
    /// @brief Get a human-readable description of the generator type and its properties.
    /// @return A string describing the generator.
    constexpr virtual std::string as_string() const { return "Not implemented"; }
    constexpr virtual ~Generator() = default;

    /// @brief Deserialize `serialized` into a Generator derived class based on data prefix.
    ///
    /// The serialized (target) class must be registered using #DESERIALIZABLE_GEN.
    ///
    /// @tparam T type of input range over bytes
    /// @param serialized byte representation of a Generator derived class
    /// @return A unique pointer owning a deserialized instance of an appropriate class
    /// @throws `std::runtime_error` if the data does not match any registered class.
    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    static std::unique_ptr<Generator> deserialize(const T &serialized)
    {
        for (const auto &[name, factory] : gen_deserializers) {
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

/// An abstract class representing a generator that is a decorator over another one.
class GeneratorDecor : public Generator {
protected:
    /// Pointer to the decorated generator.
    std::unique_ptr<Generator> m_base{};
    /// @brief Simulate own signal at a given time.
    /// @param time simulation time
    /// @return Simulation result of own generator function ignoring the decorated #m_base class.
    virtual double simulate_internal(int time) = 0;
    /// @brief Equality check with other generators.
    ///
    /// This method compares only the GeneratorDecor and Generator member variables. Derived classes
    /// with additional member variables must override this method.<br>
    /// The overrides should use the #eq method of their predecessors
    ///
    /// @param b the other generator, must be (derived from) GeneratorDecor
    /// @return `true` if the generator has the same properties
    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorDecor &>(b);
        return Generator::eq(bp) && *m_base == *bp.m_base;
    }

public:
    /// @brief Regular constructor of a decorator generator over another generator.
    /// @details If both `t_start` and `t_end` are 0, the generator is always active.
    /// @param base Non-null pointer to decorated generator
    /// @param amplitude amplitude of the generated signal
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    /// @throws std::runtime_error if `base` is `nullptr`
    constexpr GeneratorDecor(std::unique_ptr<Generator> &&base, double amplitude, int t_start = 0,
                             int t_end = 0)
        : Generator{ amplitude, t_start, t_end }
        , m_base{ std::move(base) }
    {
        if (!m_base)
            throw std::runtime_error{ "base must not be null" };
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @tparam T type of input range over bytes
    /// @param serialized input range over bytes representing serialized GeneratorDecor
    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    GeneratorDecor(const T &serialized)
        : Generator{ serialized }
    {
        auto remaining = std::ranges::drop_view{ serialized, Generator::dump_size };
        m_base = deserialize(remaining);
    }
    /// @brief Perform simulation of output at given simulation time.
    /// @param time simulation time
    /// @return Generator's simulation output including the decorated generator.
    double symuluj(int time) override { return m_base->symuluj(time) + simulate_internal(time); }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(Generator::dump(), m_base->dump());
    }

    // I don't want to make a getter for the pointer, so GeneratorsConfig must be a friend
    friend class GeneratorsConfig;
};

/// A base Generator capable of generating a constant value.
class GeneratorBaza : public Generator {
public:
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "base" };

    /// @brief Regular (and default) constructor.
    /// @details If both `t_start` and `t_end` are 0, the generator is always active.
    /// @param value signal value during activity time
    /// @param t_start start time of activity
    /// @param t_end end time of activity (inclusive)
    constexpr GeneratorBaza(double value = 0.0, int t_start = 0, int t_end = 0)
        : Generator{ value, t_start, t_end }
    {
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorBaza
    constexpr GeneratorBaza(const std::ranges::input_range auto &serialized)
        : Generator{ std::ranges::drop_view{ serialized, unique_name.size() } }
    {
        if (!prefix_match(unique_name, serialized))
            throw std::runtime_error{
                "GeneratorBaza serialized data does not start with expected prefix"
            };
    }
    /// Alias to #set_amplitude
    constexpr void set_value(double value = 0.0) noexcept { set_amplitude(value); };
    constexpr double symuluj(int time) override { return enabled_time(time) ? m_amplitude : 0.0; }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), Generator::dump());
    }
    constexpr std::string as_string() const override
    {
        return std::format(
            "Base [V={}{}]", m_amplitude,
            m_t_start == 0 && m_t_end == 0 ? "" : std::format(" <{}-{}>", m_t_start, m_t_end));
    }

    friend bool operator==(const GeneratorBaza &a, const GeneratorBaza &b)
    {
        return static_cast<const Generator &>(a) == static_cast<const Generator &>(b);
    }
};
DESERIALIZABLE_GEN(GeneratorBaza);

/// An abstract class representing a periodic generator that is a decorator over another Generator.
class GeneratorPeriodic : public GeneratorDecor {
protected:
    /// Period of the function implemented by the generator.
    uint32_t m_period;

    /// @brief Equality check with other generators.
    ///
    /// This method compares only the GeneratorPeriodic, GeneratorDecor and Generator member
    /// variables. Derived classes with additional member variables must override this method.<br>
    /// The overrides should use the #eq method of their predecessors
    ///
    /// @param b the other generator, must be (derived from) GeneratorPeriodic
    /// @return `true` if the generator has the same properties
    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorPeriodic &>(b);
        return GeneratorDecor::eq(bp) && m_period == bp.m_period;
    }

public:
    /// @brief Regular constructor of a periodic generator that decorates another Generator.
    /// @param base non-null pointer to decorated generator
    /// @param amplitude amplitude of the generated signal
    /// @param period period of the simulated function
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    constexpr GeneratorPeriodic(std::unique_ptr<Generator> &&base, double amplitude,
                                uint32_t period, int t_start = 0, int t_end = 0)
        : GeneratorDecor{ std::move(base), amplitude, t_start, t_end }
        , m_period{ period }
    {
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorPeriodic
    constexpr GeneratorPeriodic(const std::ranges::input_range auto &serialized)
        : GeneratorDecor{ std::ranges::drop_view{ serialized, sizeof m_period } }
    {
        using B = std::ranges::range_value_t<decltype(serialized)>;
        std::array<B, sizeof m_period> period_bytes;
        std::ranges::copy_n(std::ranges::begin(serialized), sizeof m_period, period_bytes.begin());
        m_period = from_bytes<decltype(m_period)>(period_bytes);
    }
    /// Period (#m_period) getter
    constexpr uint32_t get_period() const noexcept { return m_period; }
    /// Period (#m_period) setter
    constexpr void set_period(uint32_t period) noexcept { m_period = period; }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(to_bytes(m_period), GeneratorDecor::dump());
    }
};

/// Sine wave generator.
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
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "sin" };
    /// @brief Regular constructor of a sine wave generator that decorates another Generator.
    /// @param base non-null pointer to decorated generator
    /// @param amplitude amplitude of the generated signal
    /// @param period period of the simulated function
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    constexpr GeneratorSinus(std::unique_ptr<Generator> &&base, double amplitude, uint32_t period,
                             int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
    {
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorSinus
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
    constexpr std::string as_string() const override
    {
        return std::format(
            "Sine [A={} T={}{}]", m_amplitude, m_period,
            m_t_start == 0 && m_t_end == 0 ? "" : std::format(" <{}-{}>", m_t_start, m_t_end));
    }

    friend bool operator==(const GeneratorSinus &a, const GeneratorSinus &b) { return a.eq(b); }
};
DESERIALIZABLE_GEN(GeneratorSinus);

/// Square wave generator with configurable duty cycle.
class GeneratorProstokat : public GeneratorPeriodic {
private:
    /// Duty cycle in (0.0, 1.0) range
    double m_duty_cycle;

    /// @brief Check if `duty_cycle` is valid, that is between 0.0 and 1.0 (both exclusive).
    /// @param duty_cycle duty cycle value to check
    /// @throws std::runtime_error if the check fails
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
    /// @brief Equality check with other generators.
    /// @param b the other generator, **must be** (derived from) GeneratorProstokat
    /// @return `true` if the generator has the same properties
    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorProstokat &>(b);
        return GeneratorPeriodic::eq(bp) && m_duty_cycle == bp.m_duty_cycle;
    }

public:
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "pwm" };
    /// @brief Regular constructor of a square wave generator that decorates another Generator.
    /// @param base non-null pointer to decorated generator
    /// @param amplitude amplitude of the generated signal
    /// @param period period of the simulated function
    /// @param duty_cycle duty cycle of the generated signal in (0.0, 1.0) range
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    constexpr GeneratorProstokat(std::unique_ptr<Generator> &&base, double amplitude,
                                 uint32_t period, double duty_cycle, int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
        , m_duty_cycle{ duty_cycle }
    {
        validate_duty_cycle(duty_cycle);
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorProstokat
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
    /// Duty cycle (#m_duty_cycle) getter.
    constexpr double get_duty_cycle() const noexcept { return m_duty_cycle; }
    /// Duty cycle (#m_duty_cycle) setter.
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
    constexpr std::string as_string() const override
    {
        return std::format(
            "Rectangular [A={} T={} D={}%{}]", m_amplitude, m_period, m_duty_cycle * 100,
            m_t_start == 0 && m_t_end == 0 ? "" : std::format(" <{}-{}>", m_t_start, m_t_end));
    }

    friend bool operator==(const GeneratorProstokat &a, const GeneratorProstokat &b)
    {
        return a.eq(b);
    }
};
DESERIALIZABLE_GEN(GeneratorProstokat);

/// Sawtooth wave generator
class GeneratorSawtooth : public GeneratorPeriodic {
private:
    constexpr double simulate_internal(int time) override
    {
        return enabled_time(time)
            ? m_amplitude * (2 * static_cast<double>(time % m_period) / m_period - 1)
            : 0.0;
    }

public:
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "saw" };
    /// @brief Regular constructor of a sawtooth wave generator that decorates another Generator.
    /// @param base non-null pointer to decorated generator
    /// @param amplitude amplitude of the generated signal
    /// @param period period of the simulated function
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    constexpr GeneratorSawtooth(std::unique_ptr<Generator> &&base, double amplitude,
                                uint32_t period, int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
    {
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorSawtooth
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
    constexpr std::string as_string() const override
    {
        return std::format(
            "Sawtooth [A={} T={}{}]", m_amplitude, m_period,
            m_t_start == 0 && m_t_end == 0 ? "" : std::format(" <{}-{}>", m_t_start, m_t_end));
    }

    friend bool operator==(const GeneratorSawtooth &a, const GeneratorSawtooth &b)
    {
        return a.eq(b);
    }
};
DESERIALIZABLE_GEN(GeneratorSawtooth);

/// @brief Abstract template base class for random generators.
/// @tparam D distribution used by the generators
template <typename D> class GeneratorRandomBase : public GeneratorDecor {
protected:
    /// Random number distribution.
    D m_dist{};

public:
    /// @brief Regular constructor of a random generator that decorates another Generator.
    /// @param base non-null pointer to decorated generator
    /// @param amplitude amplitude of the generated signal
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    GeneratorRandomBase(std::unique_ptr<Generator> &&base, double amplitude, int t_start = 0,
                        int t_end = 0)
        : GeneratorDecor{ std::move(base), amplitude, t_start, t_end }
    {
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorRandomBase
    GeneratorRandomBase(const std::ranges::input_range auto &serialized)
        : GeneratorDecor{ serialized }
    {
    }
};

/// @brief Noise generator with uniform distribution.
/// @details The noise is generated uniformly around 0 in range @f$[-\frac{amplitude}{2},
/// \frac{amplitude}{2}]@f$.
class GeneratorUniformNoise : public GeneratorRandomBase<std::uniform_real_distribution<>> {
private:
    double simulate_internal(int) override
    {
        return 2.0 * this->m_amplitude * (this->m_dist(__rng_eng) - 0.5);
    }

public:
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "rand_uniform" };
    /// @brief Regular constructor of a uniform noise generator that decorates another Generator.
    /// @param base non-null pointer to decorated generator
    /// @param amplitude amplitude of the generated signal
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    GeneratorUniformNoise(std::unique_ptr<Generator> &&base, double amplitude, int t_start = 0,
                          int t_end = 0)
        : GeneratorRandomBase<std::uniform_real_distribution<>>{ std::move(base), amplitude,
                                                                 t_start, t_end }
    {
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorUniformNoise
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
    constexpr std::string as_string() const override
    {
        return std::format(
            "Uniform noise [A={}{}]", m_amplitude,
            m_t_start == 0 && m_t_end == 0 ? "" : std::format(" <{}-{}>", m_t_start, m_t_end));
    }

    friend bool operator==(const GeneratorUniformNoise &a, const GeneratorUniformNoise &b)
    {
        return a.eq(b);
    }
};
DESERIALIZABLE_GEN(GeneratorUniformNoise);

/// Noise generator with normal distribution.
class GeneratorNormalNoise : public GeneratorRandomBase<std::normal_distribution<>> {
private:
    /// `param_type` of the distribution.
    using pt = std::normal_distribution<>::param_type;

    /// Standard deviation of the generator.
    double m_stddev;

    double simulate_internal(int) override
    {
        return this->m_dist(__rng_eng, pt{ this->m_amplitude, m_stddev });
    }
    /// @brief Equality check with other generators.
    /// @param b the other generator, **must be** (derived from) GeneratorNormalNoise
    /// @return `true` if the generator has the same properties
    bool eq(const Generator &b) const override
    {
        auto &bp = dynamic_cast<const GeneratorNormalNoise &>(b);
        return GeneratorRandomBase<std::normal_distribution<>>::eq(bp) && m_stddev == bp.m_stddev;
    }

public:
    /// Unique name/prefix used to distinguish types in deserialization.
    static constexpr std::string_view unique_name{ "rand_normal" };
    /// @brief Regular constructor of a normal noise generator that decorates another Generator.
    /// @param base non-null pointer to decorated generator
    /// @param mean mean of the generated signal
    /// @param stddev standard deviation of the generated signal
    /// @param t_start start time of own activity
    /// @param t_end end time of own activity (inclusive)
    GeneratorNormalNoise(std::unique_ptr<Generator> &&base, double mean, double stddev,
                         int t_start = 0, int t_end = 0)
        : GeneratorRandomBase<std::normal_distribution<>>{ std::move(base), mean, t_start, t_end }
        , m_stddev{ stddev }
    {
    }
    /// @brief Deserializing constructor from a range of bytes.
    /// @param serialized input range over bytes representing serialized GeneratorNormalNoise
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
    /// Alias to #get_amplitude().
    constexpr double get_mean() const noexcept { return get_amplitude(); }
    /// Alias to #set_amplitude(double).
    constexpr void set_mean(double mean) { this->set_amplitude(mean); }
    /// Standard deviation (#m_stddev) getter.
    constexpr double get_stddev() const noexcept { return m_stddev; }
    /// Standard deviation (#m_stddev) setter.
    constexpr void set_stddev(double stddev) { m_stddev = stddev; }
    constexpr std::vector<uint8_t> dump() const override
    {
        return concat_iterables(range_to_bytes(unique_name), to_bytes(m_stddev),
                                GeneratorDecor::dump());
    }
    constexpr std::string as_string() const override
    {
        return std::format(
            "Normal noise [M={} σ={}{}]", m_amplitude, m_stddev,
            m_t_start == 0 && m_t_end == 0 ? "" : std::format(" <{}-{}>", m_t_start, m_t_end));
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
