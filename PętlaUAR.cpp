#include "PętlaUAR.hpp"

void PętlaUAR::reset(double init_val)
{
    set_init(init_val);
    for (auto &e : m_loop) {
        e->reset();
    }
}

double PętlaUAR::symuluj(double u)
{
    m_prev_result = m_closed ? u - m_prev_result : u;
    for (auto &e : m_loop) {
        m_prev_result = e->symuluj(m_prev_result);
    }
    return m_prev_result;
}

#ifdef LAB_TESTS
void UARTests::test_simple_pid_arx()
{
    std::cerr << "Simple loop with PI regulator and ARX model: ";
    PętlaUAR loop;
    loop.push_back(std::make_unique<RegulatorPID>(0.4, 2.0));
    loop.push_back(std::make_unique<ModelARX>(std::vector{ -0.4 }, std::vector{ 0.6 }, 1));
    // 7.3.15 Boolean conversions
    // > A prvalue of arithmetic, unscoped enumeration, pointer, or pointer-to-member type can be
    // > converted to a prvalue of type bool. A zero value, null pointer value, or null member
    // > pointer value is converted to false; any other value is converted to true.
    for (std::size_t i = 0; i < 30; ++i)
        std::cerr << loop.symuluj(static_cast<double>(i != 0)) << ' ';
    std::cerr << std::endl;
}

void UARTests::test_uar_serialization()
{
    using p = ObiektStatyczny::point;
    it_should_not_throw("PętlaUAR serialization", []() {
        // Setup
        PętlaUAR loop{ true, 7.525 };
        loop.push_back(std::make_unique<RegulatorPID>(0.4, 2.0));
        loop.push_back(std::make_unique<ObiektStatyczny>(p{ -1.0, -1.0 }, p{ 1.0, 1.0 }));
        loop.push_back(std::make_unique<ModelARX>(std::vector{ -0.4 }, std::vector{ 0.6 }, 1));
        auto inner_loop = std::make_unique<PętlaUAR>();
        inner_loop->push_back(std::make_unique<RegulatorPID>(0.2, 1.5, 3));
        loop.push_back(std::move(inner_loop));
        // Real test
        const auto buff = loop.dump();
        PętlaUAR loop_recreated{ buff };
        if (loop_recreated != loop)
            throw std::runtime_error{ "Loops do not match" };
        for (int i = 0; i < 10; ++i) {
            double u = (i + 3) >> 2;
            if (loop.symuluj(u) != loop_recreated.symuluj(u))
                throw std::runtime_error{ "Loop simulations do not match" };
        }
    });
}

void UARTests::run_tests()
{
    test_simple_pid_arx();
    test_uar_serialization();
}
#endif
