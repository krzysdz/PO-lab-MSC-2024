#include "RegulatorPID.h"
#include <iomanip>

std::ostream &operator<<(std::ostream &os, const RegulatorPID &m)
{
    const auto flags = os.flags();
    const auto precision = os.precision();
    const auto fill = os.fill();
    os.flags(std::ios::dec | std::ios::left | std::ios::fixed);
    os.precision(std::numeric_limits<double>::max_digits10);
    os.fill(' ');
    os << m.m_k << ' ' << m.m_ti << ' ' << m.m_td << ' ' << m.m_integral << ' ' << m.m_prev_e;
    os.fill(fill);
    os.precision(precision);
    os.flags(flags);
    return os << '\n';
}

std::istream &operator>>(std::istream &is, RegulatorPID &m)
{
    double k, ti, td, integral, prev_e;
    const auto flags = is.flags();
    is.flags(std::ios::dec | std::ios::skipws);
    is >> k >> ti >> td >> integral >> prev_e;
    m.set_k(k);
    m.set_ti(ti);
    m.set_td(td);
    m.m_integral = integral;
    m.m_prev_e = prev_e;
    is.flags(flags);
    return is;
}

void Testy_RegulatorPID::test_RegulatorP_brakPobudzenia()
{
	//Sygnatura testu:
	std::cerr << "RegP (k = 0.5) -> test zerowego pobudzenia: ";
	try
	{
		// Przygotowanie danych:
		RegulatorPID instancjaTestowa(0.5);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);      // pobudzenie modelu (tu same 0)
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy (tu same 0)
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja modelu:

		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (TESTY::porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			TESTY::raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_RegulatorPID::test_RegulatorP_skokJednostkowy()
{
	//Sygnatura testu:
	std::cerr << "RegP (k = 0.5) -> test skoku jednostkowego: ";

	try
	{
		// Przygotowanie danych:
		RegulatorPID instancjaTestowa(0.5);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);      // pobudzenie modelu
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
		for (int i = 0; i < LICZ_ITER; i++)
			sygWe[i] = !!i;
		spodzSygWy = { 0.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };

		// Symulacja modelu:
		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (TESTY::porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			TESTY::raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_RegulatorPID::test_RegulatorPI_skokJednostkowy_1()
{
	//Sygnatura testu:
	std::cerr << "RegPI (k = 0.5, TI = 1.0) -> test skoku jednostkowego nr 1: ";

	try
	{
		// Przygotowanie danych:
		RegulatorPID instancjaTestowa(0.5,1.0);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);      // pobudzenie modelu
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
		for (int i = 0; i < LICZ_ITER; i++)
			sygWe[i] = !!i;
		spodzSygWy = { 0, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5, 16.5, 17.5, 18.5, 19.5, 20.5, 21.5, 22.5, 23.5, 24.5, 25.5, 26.5, 27.5, 28.5, 29.5 };

		// Symulacja modelu:
		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (TESTY::porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			TESTY::raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_RegulatorPID::test_RegulatorPI_skokJednostkowy_2()
{
	//Sygnatura testu:
	std::cerr << "RegPI (k = 0.5, TI = 10.0) -> test skoku jednostkowego nr 2: ";

	try
	{
		// Przygotowanie danych:
		RegulatorPID instancjaTestowa(0.5, 10.0);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);      // pobudzenie modelu
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
		for (int i = 0; i < LICZ_ITER; i++)
			sygWe[i] = !!i;
		spodzSygWy = { 0, 0.6, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3, 3.1, 3.2, 3.3, 3.4 };

		// Symulacja modelu:
		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (TESTY::porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			TESTY::raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_RegulatorPID::test_RegulatorPID_skokJednostkowy()
{
	//Sygnatura testu:
	std::cerr << "RegPID (k = 0.5, TI = 10.0, TD = 0.2) -> test skoku jednostkowego: ";

	try
	{
		// Przygotowanie danych:
		RegulatorPID instancjaTestowa(0.5, 10.0, 0.2);
		constexpr size_t LICZ_ITER = 30;
		std::vector<double> sygWe(LICZ_ITER);      // pobudzenie modelu
		std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
		std::vector<double> faktSygWy(LICZ_ITER);  // faktyczna sekwencja wy

		// Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
		for (int i = 0; i < LICZ_ITER; i++)
			sygWe[i] = !!i;
		spodzSygWy = { 0, 0.8, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3, 3.1, 3.2, 3.3, 3.4 };

		// Symulacja modelu:
		for (int i = 0; i < LICZ_ITER; i++)
			faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

		// Walidacja poprawności i raport:
		if (TESTY::porownanieSekwencji(spodzSygWy, faktSygWy))
			std::cerr << "OK!\n";
		else
		{
			std::cerr << "FAIL!\n";
			TESTY::raportBleduSekwencji(spodzSygWy, faktSygWy);
		}
	}
	catch (...)
	{
		std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
	}
}

void Testy_RegulatorPID::run_tests() {
	test_RegulatorP_brakPobudzenia();
	test_RegulatorP_skokJednostkowy();
	test_RegulatorPI_skokJednostkowy_1();
	test_RegulatorPI_skokJednostkowy_2();
	test_RegulatorPID_skokJednostkowy();
}
