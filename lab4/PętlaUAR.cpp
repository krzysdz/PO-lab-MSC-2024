#include "PętlaUAR.hpp"

void PętlaUAR::reset(double init_val)
{
    set_init(0.0);
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
