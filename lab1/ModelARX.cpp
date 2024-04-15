#include "ModelARX.h"
#include <cmath>
#include <iostream>
#include <iomanip>
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

void Testy_ModelARX::raportBleduSekwencji(std::vector<double> &spodz, std::vector<double> &fakt)
{
	constexpr size_t PREC = 3;
	std::cerr << std::fixed << std::setprecision(PREC);
	std::cerr << "  Spodziewany:\t";
	for (auto &el : spodz)
		std::cerr << el << ", ";
	std::cerr << "\n  Faktyczny:\t";
	for (auto &el : fakt)
		std::cerr << el << ", ";
	std::cerr << std::endl
			  << std::endl;
}

bool Testy_ModelARX::porownanieSekwencji(std::vector<double> &spodz, std::vector<double> &fakt)
{
	constexpr double TOL = 1e-3; // tolerancja dla porównań zmiennoprzecinkowych
	bool result = fakt.size() == spodz.size();
	for (int i = 0; result && i < fakt.size(); i++)
		result = fabs(fakt[i] - spodz[i]) < TOL;
	return result;
}

void Testy_ModelARX::test_ModelARX_brakPobudzenia()
{
	// Sygnatura testu:
	std::cerr << "ModelARX (-0.4 | 0.6 | 1 | 0 ) -> test zerowego pobudzenia: ";
	try
	{
		// Przygotowanie danych:
		ModelARX instancjaTestowa({-0.4}, {0.6}, 1, 0);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);	   // pobudzenie modelu (tu same 0)
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy (tu same 0)
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja modelu:

		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_ModelARX::test_ModelARX_skokJednostkowy_1()
{
	// Sygnatura testu:
	std::cerr << "ModelARX (-0.4 | 0.6 | 1 | 0 ) -> test skoku jednostkowego nr 1: ";

	try
	{
		// Przygotowanie danych:
		ModelARX instancjaTestowa({-0.4}, {0.6}, 1, 0);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);	   // pobudzenie modelu
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
		for (int i = 0; i < LICZ_ITER; i++)
			sygWe[i] = !!i;
		spodzSygWy = {0, 0, 0.6, 0.84, 0.936, 0.9744, 0.98976, 0.995904, 0.998362, 0.999345, 0.999738, 0.999895, 0.999958, 0.999983, 0.999993, 0.999997, 0.999999, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

		// Symulacja modelu:
		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_ModelARX::test_ModelARX_skokJednostkowy_2()
{
	// Sygnatura testu:
	std::cerr << "ModelARX (-0.4 | 0.6 | 2 | 0 ) -> test skoku jednostkowego nr 2: ";

	try
	{
		// Przygotowanie danych:
		ModelARX instancjaTestowa({-0.4}, {0.6}, 2, 0);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);	   // pobudzenie modelu,
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
		for (int i = 0; i < LICZ_ITER; i++)
			sygWe[i] = !!i;
		spodzSygWy = {0, 0, 0, 0.6, 0.84, 0.936, 0.9744, 0.98976, 0.995904, 0.998362, 0.999345, 0.999738, 0.999895, 0.999958, 0.999983, 0.999993, 0.999997, 0.999999, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

		// Symulacja modelu:
		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_ModelARX::test_ModelARX_skokJednostkowy_3()
{
	// Sygnatura testu:
	std::cerr << "ModelARX (-0.4, 0.2 | 0.6, 0.3 | 2 | 0 ) -> test skoku jednostkowego nr 3: ";
	try
	{
		// Przygotowanie danych:
		ModelARX instancjaTestowa({-0.4, 0.2}, {0.6, 0.3}, 2, 0);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);	   // pobudzenie modelu,
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
		for (int i = 0; i < LICZ_ITER; i++)
			sygWe[i] = !!i;
		spodzSygWy = {0, 0, 0, 0.6, 1.14, 1.236, 1.1664, 1.11936, 1.11446, 1.12191, 1.12587, 1.12597, 1.12521, 1.12489, 1.12491, 1.12499, 1.12501, 1.12501, 1.125, 1.125, 1.125, 1.125, 1.125, 1.125, 1.125, 1.125, 1.125, 1.125, 1.125, 1.125};

		// Symulacja modelu:
		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Weryfikacja poprawności i raport:
		if (porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_ModelARX::run_tests()
{
	test_ModelARX_brakPobudzenia();
	test_ModelARX_skokJednostkowy_1();
	test_ModelARX_skokJednostkowy_2();
	test_ModelARX_skokJednostkowy_3();
	ModelARX xx({-0.4, 0.2}, {0.6, 0.3}, 2, 0);
	std::cout << "delay: " << xx.m_transport_delay << " - just proving we're friends\n";
}
