#include "ModelARX.h"
#include <cmath>
#include <numeric>

ModelARX::ModelARX(std::vector<double> &&coeff_a,
                   std::vector<double> &&coeff_b,
                   const int32_t delay, const double stddev)
	: m_mt{std::random_device{}()}
{
	set_coeff_a(std::move(coeff_a));
	set_coeff_b(std::move(coeff_b));
	set_transport_delay(delay);
	set_stddev(stddev);
}

void ModelARX::set_coeff_a(std::vector<double> &&coefficients) noexcept
{
	const auto a_elems{coefficients.size()};
	m_out_signal_mem.resize(a_elems);
	m_coeff_a = coefficients;
}

void ModelARX::set_coeff_b(std::vector<double> &&coefficients) noexcept
{
	const auto b_elems{coefficients.size()};
	m_in_signal_mem.resize(b_elems);
	m_coeff_b = coefficients;
}

void ModelARX::set_transport_delay(const int32_t delay)
{
	if (delay < 0)
		throw std::runtime_error("Delay must be >= 0");
	m_transport_delay = static_cast<uint32_t>(delay);
	m_delay_mem.resize(m_transport_delay);
}

inline void ModelARX::set_stddev(const double stddev)
{
	if (!std::isfinite(stddev) || stddev < 0.0)
	{
		throw std::runtime_error{"Standard deviation must be finite and nonnegative"};
	}
	m_distribution.param(typename decltype(m_distribution)::param_type{0.0, stddev});
}

double ModelARX::symuluj(double u)
{
	m_in_signal_mem.pop_back();
	m_in_signal_mem.push_front(m_delay_mem.back());
	m_delay_mem.pop_back();
	m_delay_mem.push_front(u);
	const auto b_poly{std::inner_product(m_coeff_b.begin(), m_coeff_b.end(), m_in_signal_mem.begin(), 0.0)};
	const auto a_poly{std::inner_product(m_coeff_a.begin(), m_coeff_a.end(), m_out_signal_mem.begin(), 0.0)};
	const double noise{m_distribution(m_mt)};
	const auto y{b_poly - a_poly + noise};
	m_out_signal_mem.pop_back();
	m_out_signal_mem.push_front(y);
	return y;
}
