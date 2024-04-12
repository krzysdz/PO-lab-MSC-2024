#include "ModelARX.h"
#include <cmath>

template <std::floating_point T, std::unsigned_integral DT>
constexpr inline ModelARX<T, DT>::ModelARX(std::vector<T> &&coeff_a,
                                 std::vector<T> &&coeff_b,
								 const DT delay, const T stddev)
								 : m_mt{std::random_device{}()}
{
	set_coeff_a(std::move(coeff_a));
	set_coeff_b(std::move(coeff_b));
	set_transport_delay(delay);
	set_stddev(stddev);
}

template <std::floating_point T, std::unsigned_integral DT>
constexpr void ModelARX<T, DT>::set_coeff_a(std::vector<T> &&coefficients) noexcept
{
	const auto a_elems{coefficients.size()};
	m_out_signal_mem.resize(a_elems);
	m_coeff_a = coefficients;
}

template <std::floating_point T, std::unsigned_integral DT>
constexpr void ModelARX<T, DT>::set_coeff_b(std::vector<T> &&coefficients) noexcept
{
	const auto b_elems{coefficients.size()};
	m_in_signal_mem.resize(b_elems);
	m_coeff_b = coefficients;
}

template <std::floating_point T, std::unsigned_integral DT>
constexpr void ModelARX<T, DT>::set_transport_delay(const DT delay)
{
	m_transport_delay = delay;
	m_delay_mem.resize(delay);
}

template <std::floating_point T, std::unsigned_integral DT>
constexpr inline void ModelARX<T, DT>::set_stddev(const T stddev)
{
	if (!std::isfinite(stddev) || stddev < 0.0) {
		throw std::runtime_error{"Standard deviation must be finite and positive"};
	}
	m_distribution.param(typename decltype(m_distribution)::param_type{0.0, stddev});
}

template <std::floating_point T, std::unsigned_integral DT>
constexpr T ModelARX<T, DT>::symuluj(T u)
{
	m_in_signal_mem.pop_back();
	m_in_signal_mem.push_front(m_delay_mem.back());
	m_delay_mem.pop_back();
	m_delay_mem.push_front(u);
	const auto b_poly{std::inner_product(m_coeff_b.begin(), m_coeff_b.end(), m_in_signal_mem.begin(), T{})};
	const auto a_poly{std::inner_product(m_coeff_a.begin(), m_coeff_a.end(), m_out_signal_mem.begin(), T{})};
	const T noise{m_distribution(m_mt)};
	const auto y{b_poly - a_poly + noise};
	m_out_signal_mem.pop_back();
	m_out_signal_mem.push_front(y);
	return y;
}
