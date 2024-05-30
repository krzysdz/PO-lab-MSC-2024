#pragma once
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <stdexcept>

class Generator {
protected:
    double m_amplitude;
    int m_t_start;
    int m_t_end;

    constexpr bool enabled_time(int time) const noexcept
    {
        return (m_t_start == 0 && m_t_end == 0) || (time >= m_t_start && time <= m_t_end);
    }
    constexpr void validate_time() const
    {
        if (m_t_end < m_t_start)
            throw std::runtime_error{ "t_end cannot be smaller than t_start" };
    }

public:
    constexpr Generator(double amplitude, int t_start = 0, int t_end = 0)
        : m_amplitude{ amplitude }
        , m_t_start{ t_start }
        , m_t_end{ t_end }
    {
        validate_time();
    }
    constexpr void set_amplitude(double amplitude) { m_amplitude = amplitude; }
    constexpr void set_activity_time(int t_start, int t_end)
    {
        m_t_start = t_start;
        m_t_end = t_end;
        validate_time();
    }
    virtual double symuluj(int time) = 0;
    constexpr virtual ~Generator(){};
};

class GeneratorDecor : public Generator {
protected:
    std::unique_ptr<Generator> m_base{};
    virtual double simulate_internal(int time) = 0;

public:
    constexpr GeneratorDecor(std::unique_ptr<Generator> &&base, double amplitude, int t_start = 0,
                             int t_end = 0)
        : Generator{ amplitude, t_start, t_end }
        , m_base{ std::move(base) }
    {
        if (!m_base)
            throw std::runtime_error{ "base must not be null" };
    }
    double symuluj(int time) override { return m_base->symuluj(time) + simulate_internal(time); }
};

class GeneratorBaza : public Generator {
public:
    constexpr GeneratorBaza(double value = 0.0, int t_start = 0, int t_end = 0)
        : Generator{ value, t_start, t_end }
    {
    }
    constexpr void set_value(double value = 0.0) noexcept { set_amplitude(value); };
    constexpr double symuluj(int time) override { return enabled_time(time) ? m_amplitude : 0.0; }
};

class GeneratorPeriodic : public GeneratorDecor {
protected:
    uint32_t m_period;

public:
    constexpr GeneratorPeriodic(std::unique_ptr<Generator> &&base, double amplitude,
                                uint32_t period, int t_start = 0, int t_end = 0)
        : GeneratorDecor{ std::move(base), amplitude, t_start, t_end }
        , m_period{ period }
    {
    }
    constexpr void set_period(uint32_t period) noexcept { m_period = period; }
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
    constexpr GeneratorSinus(std::unique_ptr<Generator> &&base, double amplitude, uint32_t period,
                             int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
    {
    }
};

class GeneratorProstokat : public GeneratorPeriodic {
private:
    double m_duty_cycle;

    constexpr void validate_duty_cycle() const
    {
        if (m_duty_cycle >= 1.0 || m_duty_cycle <= 0.0 || !std::isfinite(m_duty_cycle))
            throw std::runtime_error{ "Duty cycle should be between 0 and 1. If you want a "
                                      "constant signal use GeneratorBaza." };
    }
    constexpr double simulate_internal(int time) override
    {
        return enabled_time(time) && (time % m_period) < m_duty_cycle * m_period ? m_amplitude
                                                                                 : 0.0;
    }

public:
    constexpr GeneratorProstokat(std::unique_ptr<Generator> &&base, double amplitude,
                                 uint32_t period, double duty_cycle, int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
        , m_duty_cycle{ duty_cycle }
    {
        validate_duty_cycle();
    }
    constexpr void set_duty_cycle(double duty_cycle)
    {
        m_duty_cycle = duty_cycle;
        validate_duty_cycle();
    }
};

class GeneratorSawtooth : public GeneratorPeriodic {
private:
    constexpr double simulate_internal(int time) override
    {
        return enabled_time(time)
            ? m_amplitude * (2 * static_cast<double>(time % m_period) / m_period - 1)
            : 0.0;
    }

public:
    constexpr GeneratorSawtooth(std::unique_ptr<Generator> &&base, double amplitude,
                                uint32_t period, int t_start = 0, int t_end = 0)
        : GeneratorPeriodic{ std::move(base), amplitude, period, t_start, t_end }
    {
    }
};

template <std::uniform_random_bit_generator G, typename D>
class GeneratorRandomBase : public GeneratorDecor {
protected:
    D m_dist{};
    std::shared_ptr<G> m_gen_p;

    void validate_gen_ptr() const
    {
        if (!m_gen_p)
            throw std::runtime_error{ "Pointer to generator is null" };
    }

public:
    GeneratorRandomBase(std::unique_ptr<Generator> &&base, double amplitude,
                        const std::shared_ptr<G> &gen_p, int t_start = 0, int t_end = 0)
        : GeneratorDecor{ std::move(base), amplitude, t_start, t_end }
        , m_gen_p{ gen_p }
    {
        validate_gen_ptr();
    }
    void set_generator(std::shared_ptr<G> &gen_p)
    {
        m_gen_p = gen_p;
        validate_gen_ptr();
    }
};

template <std::uniform_random_bit_generator G>
class GeneratorUniformNoise : public GeneratorRandomBase<G, std::uniform_real_distribution<>> {
private:
    double simulate_internal(int) override
    {
        return this->m_amplitude * (this->m_dist(*this->m_gen_p) - 0.5);
    }

public:
    GeneratorUniformNoise(std::unique_ptr<Generator> &&base, double amplitude,
                          const std::shared_ptr<G> &gen_p, int t_start = 0, int t_end = 0)
        : GeneratorRandomBase<G, std::uniform_real_distribution<>>{ std::move(base), amplitude,
                                                                    gen_p, t_start, t_end }
    {
    }
};

template <std::uniform_random_bit_generator G>
class GeneratorNormalNoise : public GeneratorRandomBase<G, std::normal_distribution<>> {
private:
    using pt = std::normal_distribution<>::param_type;

    double m_stddev;

    double simulate_internal(int) override
    {
        return this->m_dist(*this->m_gen_p, pt{ this->m_amplitude, m_stddev });
    }

public:
    GeneratorNormalNoise(std::unique_ptr<Generator> &&base, double mean, double stddev,
                         const std::shared_ptr<G> &gen_p, int t_start = 0, int t_end = 0)
        : GeneratorRandomBase<G, std::normal_distribution<>>{ std::move(base), mean, gen_p, t_start,
                                                              t_end }
        , m_stddev{ stddev }
    {
    }
    constexpr void set_mean(double mean) { this->set_amplitude(mean); }
    constexpr void set_stddev(double stddev) { m_stddev = stddev; }
};
