#include "generators.hpp"

std::mt19937_64 __rng_eng{ std::random_device{}() };
std::vector<std::pair<std::vector<std::uint8_t>,
                      std::unique_ptr<Generator> (*)(const std::vector<std::uint8_t> &)>>
    gen_deserializers;

#ifdef LAB_TESTS
#include <format>

auto GeneratorTests::get_base() { return std::make_unique<GeneratorBaza>(); }
void GeneratorTests::test_base()
{
    it_should_not_throw("Base generator"sv, []() {
        for (double v : { 0.0, 1.5, 13.2, -7.3 }) {
            GeneratorBaza b{ v };
            for (int t : { 1, 15, 20, 123456 }) {
                auto r = b.symuluj(t);
                if (r != v)
                    throw std::logic_error{ std::format(
                        "Generator({0}).symuluj({1}) returned {2} instead of {0}", v, t, r) };
            }
        }
    });
}
void GeneratorTests::test_activity_time()
{
    constexpr double v = 2.0;
    // The view is materialized after the `test` lambda is executed, so `b` must be still alive.
    // If `b` was declared inside the lambda, a stack-use-after-scope would occur during
    // comparison. Alternatively std::ranges::to could be used to materialize the view into a
    // container.
    GeneratorBaza b{ v, 2, 4 };
    const auto expected = { 0.0, 0.0, v, v, v, 0.0 };
    it_should_return("Activity time (Base generator)"sv, expected, [&] {
        return std::views::iota(0UL, std::ranges::size(expected))
            | std::views::transform([&b](auto t) mutable -> double { return b.symuluj(t); });
    });
}
void GeneratorTests::test_sin()
{
    constexpr double amplitude = 2.5;
    constexpr uint32_t period = 20;
    const auto expected = read_test_data("./tests/sin.csv", 3);
    GeneratorSinus s{ get_base(), amplitude, period };
    it_should_return(
        "Sine generator"sv, std::views::values(expected),
        [&] {
            return std::views::keys(expected)
                | std::views::transform([&s](auto t) mutable { return s.symuluj(t); });
        },
        floating_eq<double, 1e-13>);
}
void GeneratorTests::test_rect()
{
    constexpr double z = 0.0;
    constexpr double a = 1.75;
    constexpr uint32_t period = 10;
    constexpr double duty_cycle = 0.2;
    const auto expected = { a, a, z, z, z, z, z, z, z, z, a, a, z, z, z, z };
    GeneratorProstokat pwm_gen{ get_base(), a, period, duty_cycle };
    it_should_return("PWM generator"sv, expected, [&] {
        return std::views::iota(0UL, std::ranges::size(expected))
            | std::views::transform(
                   [&pwm_gen](auto t) mutable -> double { return pwm_gen.symuluj(t); });
    });
    constexpr auto err_duty_cycle
        = "Duty cycle should be between 0 and 1. If you want a constant signal use GeneratorBaza."sv;
    it_should_throw<std::runtime_error>(
        "PWM generator - negative duty cycle",
        [] { GeneratorProstokat{ get_base(), a, period, -0.3 }; }, err_duty_cycle);
    it_should_throw<std::runtime_error>(
        "PWM generator - duty cycle >= 1", [] { GeneratorProstokat{ get_base(), a, period, 1.0 }; },
        err_duty_cycle);
}
void GeneratorTests::test_sawtooth()
{
    constexpr double amplitude = 0.625;
    constexpr uint32_t period = 40;
    const auto expected = read_test_data("./tests/sawtooth.csv", 3);
    GeneratorSawtooth s{ get_base(), amplitude, period };
    it_should_return(
        "Sawtooth generator"sv, std::views::values(expected),
        [&] {
            return std::views::keys(expected)
                | std::views::transform([&s](auto t) mutable { return s.symuluj(t); });
        },
        floating_eq<double, 1e-14>);
}
void GeneratorTests::test_addition()
{
    constexpr double ba = 1;
    constexpr double ra = 0.75;
    constexpr double bo = ba + ra;
    constexpr uint32_t period = 8;
    constexpr double duty_cycle = 0.25;
    constexpr int start = 1;
    constexpr int end = 16;
    // clang-format off
        const auto expected
            = { ba, bo, ba, ba, ba, ba, ba, ba,
                bo, bo, ba, ba, ba, ba, ba, ba,
                bo, ba, ba };
    // clang-format on
    GeneratorProstokat gen{
        std::make_unique<GeneratorBaza>(ba), ra, period, duty_cycle, start, end
    };
    it_should_return("Signal addition"sv, expected, [&] {
        return std::views::iota(0UL, std::ranges::size(expected))
            | std::views::transform([&gen](auto t) mutable -> double { return gen.symuluj(t); });
    });
}
void GeneratorTests::test_serialization()
{
    it_should_not_throw("GeneratorBaza serialization", [] {
        GeneratorBaza b1{ 2.5, 1, 8 };
        const auto serialized1 = b1.dump();
        GeneratorBaza b2{ serialized1 };
        const auto serialized2 = b2.dump();
        if (b1 != b2 || serialized1 != serialized2)
            throw std::runtime_error{ std::format("b1 == b2: {}; serialized1 == serialized2: {}",
                                                  b1 == b2, serialized1 == serialized2) };
    });
    it_should_not_throw("GeneratorSinus serialization", [] {
        GeneratorSinus s1{ std::make_unique<GeneratorBaza>(2.5, 1, 8), 1.75, 64, 3, 100 };
        const auto serialized1 = s1.dump();
        GeneratorSinus s2{ serialized1 };
        const auto serialized2 = s2.dump();
        if (s1 != s2 || serialized1 != serialized2)
            throw std::runtime_error{ std::format("s1 == s2: {}; serialized1 == serialized2: {}",
                                                  s1 == s2, serialized1 == serialized2) };
    });
    it_should_not_throw("GeneratorProstokat serialization", [] {
        GeneratorProstokat p1{
            std::make_unique<GeneratorBaza>(2.5, 1, 8), 1.75, 64, 0.625, 3, 100
        };
        const auto serialized1 = p1.dump();
        GeneratorProstokat p2{ serialized1 };
        const auto serialized2 = p2.dump();
        if (p1 != p2 || serialized1 != serialized2)
            throw std::runtime_error{ std::format("p1 == p2: {}; serialized1 == serialized2: {}",
                                                  p1 == p2, serialized1 == serialized2) };
    });
    it_should_not_throw("GeneratorSawtooth serialization", [] {
        GeneratorSawtooth s1{ std::make_unique<GeneratorBaza>(2.5, 1, 8), 1.75, 64, 3, 100 };
        const auto serialized1 = s1.dump();
        GeneratorSawtooth s2{ serialized1 };
        const auto serialized2 = s2.dump();
        if (s1 != s2 || serialized1 != serialized2)
            throw std::runtime_error{ std::format("s1 == s2: {}; serialized1 == serialized2: {}",
                                                  s1 == s2, serialized1 == serialized2) };
    });
    it_should_not_throw("GeneratorUniformNoise serialization", [] {
        GeneratorUniformNoise u1{ std::make_unique<GeneratorBaza>(2.5, 1, 8), 1.75, 3, 100 };
        const auto serialized1 = u1.dump();
        GeneratorUniformNoise u2{ serialized1 };
        const auto serialized2 = u2.dump();
        if (u1 != u2 || serialized1 != serialized2)
            throw std::runtime_error{ std::format("u1 == u2: {}; serialized1 == serialized2: {}",
                                                  u1 == u2, serialized1 == serialized2) };
    });
    it_should_not_throw("GeneratorNormalNoise serialization", [] {
        GeneratorNormalNoise n1{ std::make_unique<GeneratorBaza>(2.5, 1, 8), 1.75, 0.243, 3, 100 };
        const auto serialized1 = n1.dump();
        GeneratorNormalNoise n2{ serialized1 };
        const auto serialized2 = n2.dump();
        if (n1 != n2 || serialized1 != serialized2)
            throw std::runtime_error{ std::format("n1 == n2: {}; serialized1 == serialized2: {}",
                                                  n1 == n2, serialized1 == serialized2) };
    });
    it_should_not_throw("Stacked serialization", [] {
        auto base = std::make_unique<GeneratorBaza>(2.5, 1, 8);
        auto saw = std::make_unique<GeneratorSawtooth>(std::move(base), 123.25, 87, 12, 398);
        auto sin = std::make_unique<GeneratorSinus>(std::move(saw), 2.125, 55, 1, 974);
        auto pwm = std::make_unique<GeneratorProstokat>(std::move(sin), 63.75, 285, 0.75, 2, 645);
        auto uni = std::make_unique<GeneratorUniformNoise>(std::move(pwm), 5.35, 35, 48);
        auto norm = std::make_unique<GeneratorNormalNoise>(std::move(uni), 0.558, 1.6, 91, 834);
        GeneratorSinus last{ std::move(norm), 0.315, 4315, 75, 622 };
        const auto serialized_last = last.dump();
        GeneratorSinus restored_last{ serialized_last };
        const auto serialized_restored = restored_last.dump();
        if (last != restored_last)
            throw std::runtime_error{ "last != restored_last" };
        if (serialized_last != serialized_restored)
            throw std::runtime_error{ "serialized_last != serialized_restored" };
    });
    it_should_not_throw("Stacked serialization - Generator::serialize()", [] {
        auto base = std::make_unique<GeneratorBaza>(2.5, 1, 8);
        auto saw = std::make_unique<GeneratorSawtooth>(std::move(base), 123.25, 87, 12, 398);
        auto sin = std::make_unique<GeneratorSinus>(std::move(saw), 2.125, 55, 1, 974);
        auto pwm = std::make_unique<GeneratorProstokat>(std::move(sin), 63.75, 285, 0.75, 2, 645);
        auto uni = std::make_unique<GeneratorUniformNoise>(std::move(pwm), 5.35, 35, 48);
        auto norm = std::make_unique<GeneratorNormalNoise>(std::move(uni), 0.558, 1.6, 91, 834);
        auto last = std::make_unique<GeneratorSinus>(std::move(norm), 0.315, 4315, 75, 622);
        const auto serialized_last = last->dump();
        auto restored_last = Generator::deserialize(serialized_last);
        const auto serialized_restored = restored_last->dump();
        if (*last != *restored_last)
            throw std::runtime_error{ "*last != *restored_last" };
        if (serialized_last != serialized_restored)
            throw std::runtime_error{ "serialized_last != serialized_restored" };
    });
}

void GeneratorTests::run_tests()
{
    test_base();
    test_activity_time();
    test_sin();
    test_rect();
    test_sawtooth();
    test_addition();
    test_serialization();
}
#endif
