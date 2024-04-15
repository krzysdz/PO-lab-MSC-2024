#pragma once
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <random>
#include <stdexcept>
#include <vector>

#include "ObiektSISO.hpp"

class ModelARX : public ObiektSISO
{
private:
	std::vector<double> m_coeff_a;
	std::vector<double> m_coeff_b;
	uint32_t m_transport_delay;
	std::normal_distribution<double> m_distribution;
	std::deque<double> m_in_signal_mem;
	std::deque<double> m_out_signal_mem;
	std::deque<double> m_delay_mem;
	std::mt19937_64 m_mt;

public:
	ModelARX() = delete;
	ModelARX(std::vector<double> &&coeff_a, std::vector<double> &&coeff_b, const int32_t delay = 1,
	                   const double stddev = 0.0);
	void set_coeff_a(std::vector<double> &&coefficients) noexcept;
	void set_coeff_b(std::vector<double> &&coefficients) noexcept;
	void set_transport_delay(const int32_t delay);
	void set_stddev(const double stddev);
	double symuluj(double u) override;
	~ModelARX() = default;
};