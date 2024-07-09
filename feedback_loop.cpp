#include "feedback_loop.hpp"

double feedback_step(RegulatorPID &regulator, ModelARX &model, const double u,
                     const std::optional<double> &reset)
{
    static double last_output = 0.0;
    if (reset.has_value())
        last_output = reset.value();
    const auto e = u - last_output;
    const auto input = regulator.symuluj(e);
    last_output = model.symuluj(input);
    return last_output;
}

#ifdef LAB_TESTS
ModelARX FeedbackTests::get_model() { return ModelARX{ { -0.4 }, { 0.6 }, 1, 0.0 }; }

void FeedbackTests::run_sim(RegulatorPID &pid)
{
    constexpr std::size_t steps{ 30 };
    auto m = get_model();
    std::cout << feedback_step(pid, m, 0.0, 0.0);
    for (auto i = 1UL; i < steps; ++i)
        std::cout << ' ' << feedback_step(pid, m, 1.0);
    std::cout << std::endl;
}

void FeedbackTests::simulate_p_1()
{
    std::cout << "P regulator [k = 0.5]\n";
    RegulatorPID p{ 0.5 };
    run_sim(p);
}

void FeedbackTests::simulate_p_2()
{
    std::cout << "P regulator [k = 2.0]\n";
    RegulatorPID p{ 2.0 };
    run_sim(p);
}

void FeedbackTests::simulate_pi_1()
{
    std::cout << "PI regulator [k = 0.5, Ti = 10.0]\n";
    RegulatorPID pi{ 0.5, 10.0 };
    run_sim(pi);
}

void FeedbackTests::simulate_pi_2()
{
    std::cout << "PI regulator [k = 0.4, Ti = 2.0]\n";
    RegulatorPID pi{ 0.4, 2.0 };
    run_sim(pi);
}

void FeedbackTests::run_tests()
{
    simulate_p_1();
    simulate_p_2();
    simulate_pi_1();
    simulate_pi_2();
}
#endif
