#include "ModelARX.h"
#include "util.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <version>
#if __cpp_lib_format >= 201907L
#include <format>
#endif

double ModelARX::get_random()
{
    const auto r = m_distribution(m_mt);
    m_n_generated++;
    return r;
}

template <typename Iter>
    requires std::contiguous_iterator<Iter>
    && std::is_same_v<typename std::iterator_traits<Iter>::value_type, uint8_t>
ModelARX::ModelARX(Iter start, Iter end)
{
    const auto data_size{ std::distance(start, end) };
    if (data_size < static_cast<std::ptrdiff_t>(sizeof(raw_data_t)))
        throw std::runtime_error{ "Data size is smaller than constant-length part" };

    const auto raw_data_p = reinterpret_cast<const raw_data_t *>(&*start);
    const auto expected_size{ static_cast<std::ptrdiff_t>(
        (raw_data_p->n_coeff_a + raw_data_p->n_coeff_b + raw_data_p->in_n + raw_data_p->out_n
         + raw_data_p->delay_n)
            * 8
        + sizeof(raw_data_t)) };
    if (data_size != expected_size)
        throw std::runtime_error{
#if __cpp_lib_format >= 201907L
            std::format("Data size ({} bytes) does not match the expected size ({} bytes)",
                        data_size, expected_size)
#else
            "Data size does not match the expected size"
#endif
        };

    auto vec_start = reinterpret_cast<const double *>(&*start + sizeof(raw_data_t));
    auto vec_end = vec_start + raw_data_p->n_coeff_a;
    m_coeff_a = std::vector<double>(vec_start, vec_end);
    vec_start = vec_end;
    vec_end += raw_data_p->n_coeff_b;
    m_coeff_b = std::vector<double>(vec_start, vec_end);
    vec_start = vec_end;
    vec_end += raw_data_p->in_n;
    m_in_signal_mem = decltype(m_in_signal_mem)(vec_start, vec_end);
    vec_start = vec_end;
    vec_end += raw_data_p->out_n;
    m_out_signal_mem = decltype(m_out_signal_mem)(vec_start, vec_end);
    vec_start = vec_end;
    vec_end += raw_data_p->delay_n;
    m_delay_mem = decltype(m_delay_mem)(vec_start, vec_end);

    m_transport_delay = static_cast<uint32_t>(raw_data_p->delay_n);
    m_init_seed = raw_data_p->init_seed;
    // Restore generator state by initializing it with original seed and skipping the same number of
    // results. It may be inefficient, but the STL implementation of Mersenne Twister does not
    // expose the internal state and I don't want to use implementation-specific solutions, which
    // could break in other versions of libstdc++, libc++ and MSVC STL.
    // m_mt.discard(m_n_generated) won't work, because the distribution has its own state...
    m_mt = std::mt19937_64{ static_cast<decltype(m_mt)::result_type>(m_init_seed) };
    m_distribution = decltype(m_distribution)(raw_data_p->dist_mean, raw_data_p->dist_stddev);
    while (m_n_generated < raw_data_p->n_generated)
        get_random();
}

ModelARX::ModelARX(std::vector<double> &&coeff_a, std::vector<double> &&coeff_b,
                   const int32_t delay, const double stddev)
    : m_init_seed{ std::random_device{}() }
    , m_mt{ static_cast<decltype(m_mt)::result_type>(m_init_seed) }
{
    set_coeff_a(std::move(coeff_a));
    set_coeff_b(std::move(coeff_b));
    set_transport_delay(delay);
    set_stddev(stddev);
}

void ModelARX::set_coeff_a(std::vector<double> &&coefficients) noexcept
{
    const auto a_elems{ coefficients.size() };
    m_out_signal_mem.resize(a_elems);
    m_coeff_a = coefficients;
}

void ModelARX::set_coeff_b(std::vector<double> &&coefficients) noexcept
{
    const auto b_elems{ coefficients.size() };
    m_in_signal_mem.resize(b_elems);
    m_coeff_b = coefficients;
}

void ModelARX::set_transport_delay(const int32_t delay)
{
    if (delay < 0)
        throw std::runtime_error{ "Delay must be >= 0" };
    m_transport_delay = static_cast<uint32_t>(delay);
    m_delay_mem.resize(m_transport_delay);
}

inline void ModelARX::set_stddev(const double stddev)
{
    if (!std::isfinite(stddev) || stddev < 0.0) {
        throw std::runtime_error{ "Standard deviation must be finite and nonnegative" };
    }
    m_distribution.param(typename decltype(m_distribution)::param_type{ 0.0, stddev });
}

double ModelARX::symuluj(double u)
{
    m_in_signal_mem.pop_back();
    m_in_signal_mem.push_front(m_delay_mem.back());
    m_delay_mem.pop_back();
    m_delay_mem.push_front(u);
    const auto b_poly{ std::inner_product(m_coeff_b.begin(), m_coeff_b.end(),
                                          m_in_signal_mem.begin(), 0.0) };
    const auto a_poly{ std::inner_product(m_coeff_a.begin(), m_coeff_a.end(),
                                          m_out_signal_mem.begin(), 0.0) };
    const double noise{ get_random() };
    const auto y{ b_poly - a_poly + noise };
    m_out_signal_mem.pop_back();
    m_out_signal_mem.push_front(y);
    return y;
}

std::vector<uint8_t> ModelARX::dump() const
{
    const uint64_t n_coeff_a{ m_coeff_a.size() };
    const uint64_t n_coeff_b{ m_coeff_b.size() };
    const uint64_t in_n{ m_in_signal_mem.size() };
    const uint64_t out_n{ m_out_signal_mem.size() };
    const uint64_t delay_n{ m_delay_mem.size() };
    const double dist_mean{ m_distribution.mean() };
    const double dist_stddev{ m_distribution.stddev() };
    // It's easier to deal with a whole struct instead of separate variables
    const raw_data_t raw{ n_coeff_a, n_coeff_b, dist_mean,   dist_stddev,  in_n,
                          out_n,     delay_n,   m_init_seed, m_n_generated };
    // Prepare the output buffer and check some assumptions about its size
    std::size_t dump_size{ (n_coeff_a + n_coeff_b + in_n + out_n + delay_n) * 8 + sizeof(raw) };
    static_assert(sizeof(raw) == 9U * 8U);
    static_assert(sizeof(double) == 8U);
    std::vector<uint8_t> serialized(dump_size);
    // Write everything to the buffer
    auto out_ptr{ serialized.data() };
    std::memcpy(out_ptr, reinterpret_cast<const uint8_t *>(&raw), sizeof(raw));
    out_ptr += sizeof(raw);
    std::memcpy(out_ptr, reinterpret_cast<const uint8_t *>(m_coeff_a.data()), n_coeff_a * 8);
    out_ptr += n_coeff_a * 8;
    std::memcpy(out_ptr, reinterpret_cast<const uint8_t *>(m_coeff_b.data()), n_coeff_b * 8);
    out_ptr += n_coeff_b * 8;
    std::copy(m_in_signal_mem.begin(), m_in_signal_mem.end(), reinterpret_cast<double *>(out_ptr));
    out_ptr += in_n * 8;
    std::copy(m_out_signal_mem.begin(), m_out_signal_mem.end(),
              reinterpret_cast<double *>(out_ptr));
    out_ptr += out_n * 8;
    std::copy(m_delay_mem.begin(), m_delay_mem.end(), reinterpret_cast<double *>(out_ptr));
    out_ptr += delay_n * 8;
    // Final size check
    if (out_ptr != std::to_address(serialized.end()))
        throw std::runtime_error{ "Serialization is broken" };

    return serialized;
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
    std::cerr << std::endl << std::endl;
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
    try {
        // Przygotowanie danych:
        ModelARX instancjaTestowa({ -0.4 }, { 0.6 }, 1, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER); // pobudzenie modelu (tu same 0)
        std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy (tu same 0)
        std::vector<double> faktSygWy(LICZ_ITER); // faktyczna sekwencja wy

        // Symulacja modelu:

        for (int i = 0; i < LICZ_ITER; i++)
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

        // Walidacja poprawności i raport:
        if (porownanieSekwencji(spodzSygWy, faktSygWy))
            std::cerr << "OK!\n";
        else {
            std::cerr << "FAIL!\n";
            raportBleduSekwencji(spodzSygWy, faktSygWy);
        }
    } catch (...) {
        std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
    }
}

void Testy_ModelARX::test_ModelARX_skokJednostkowy_1()
{
    // Sygnatura testu:
    std::cerr << "ModelARX (-0.4 | 0.6 | 1 | 0 ) -> test skoku jednostkowego nr 1: ";

    try {
        // Przygotowanie danych:
        ModelARX instancjaTestowa({ -0.4 }, { 0.6 }, 1, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER); // pobudzenie modelu
        std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
        std::vector<double> faktSygWy(LICZ_ITER); // faktyczna sekwencja wy

        // Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
        for (int i = 0; i < LICZ_ITER; i++)
            sygWe[i] = !!i;
        spodzSygWy
            = { 0,        0,        0.6,      0.84,     0.936,    0.9744,   0.98976,  0.995904,
                0.998362, 0.999345, 0.999738, 0.999895, 0.999958, 0.999983, 0.999993, 0.999997,
                0.999999, 1,        1,        1,        1,        1,        1,        1,
                1,        1,        1,        1,        1,        1 };

        // Symulacja modelu:
        for (int i = 0; i < LICZ_ITER; i++)
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

        // Walidacja poprawności i raport:
        if (porownanieSekwencji(spodzSygWy, faktSygWy))
            std::cerr << "OK!\n";
        else {
            std::cerr << "FAIL!\n";
            raportBleduSekwencji(spodzSygWy, faktSygWy);
        }
    } catch (...) {
        std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
    }
}

void Testy_ModelARX::test_ModelARX_skokJednostkowy_2()
{
    // Sygnatura testu:
    std::cerr << "ModelARX (-0.4 | 0.6 | 2 | 0 ) -> test skoku jednostkowego nr 2: ";

    try {
        // Przygotowanie danych:
        ModelARX instancjaTestowa({ -0.4 }, { 0.6 }, 2, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER); // pobudzenie modelu,
        std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
        std::vector<double> faktSygWy(LICZ_ITER); // faktyczna sekwencja wy

        // Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
        for (int i = 0; i < LICZ_ITER; i++)
            sygWe[i] = !!i;
        spodzSygWy
            = { 0,        0,        0,        0.6,      0.84,     0.936,    0.9744,   0.98976,
                0.995904, 0.998362, 0.999345, 0.999738, 0.999895, 0.999958, 0.999983, 0.999993,
                0.999997, 0.999999, 1,        1,        1,        1,        1,        1,
                1,        1,        1,        1,        1,        1 };

        // Symulacja modelu:
        for (int i = 0; i < LICZ_ITER; i++)
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

        // Walidacja poprawności i raport:
        if (porownanieSekwencji(spodzSygWy, faktSygWy))
            std::cerr << "OK!\n";
        else {
            std::cerr << "FAIL!\n";
            raportBleduSekwencji(spodzSygWy, faktSygWy);
        }
    } catch (...) {
        std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
    }
}

void Testy_ModelARX::test_ModelARX_skokJednostkowy_3()
{
    // Sygnatura testu:
    std::cerr << "ModelARX (-0.4, 0.2 | 0.6, 0.3 | 2 | 0 ) -> test skoku jednostkowego nr 3: ";
    try {
        // Przygotowanie danych:
        ModelARX instancjaTestowa({ -0.4, 0.2 }, { 0.6, 0.3 }, 2, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER); // pobudzenie modelu,
        std::vector<double> spodzSygWy(LICZ_ITER); // spodziewana sekwencja wy
        std::vector<double> faktSygWy(LICZ_ITER); // faktyczna sekwencja wy

        // Symulacja skoku jednostkowego w chwili 1. (!!i - daje 1 dla i != 0);
        for (int i = 0; i < LICZ_ITER; i++)
            sygWe[i] = !!i;
        spodzSygWy = { 0,       0,       0,       0.6,     1.14,    1.236,   1.1664,  1.11936,
                       1.11446, 1.12191, 1.12587, 1.12597, 1.12521, 1.12489, 1.12491, 1.12499,
                       1.12501, 1.12501, 1.125,   1.125,   1.125,   1.125,   1.125,   1.125,
                       1.125,   1.125,   1.125,   1.125,   1.125,   1.125 };

        // Symulacja modelu:
        for (int i = 0; i < LICZ_ITER; i++)
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);

        // Weryfikacja poprawności i raport:
        if (porownanieSekwencji(spodzSygWy, faktSygWy))
            std::cerr << "OK!\n";
        else {
            std::cerr << "FAIL!\n";
            raportBleduSekwencji(spodzSygWy, faktSygWy);
        }
    } catch (...) {
        std::cerr << "INTERUPTED! (niespodziwany wyjatek)\n";
    }
}

ModelARX Testy_ModelARX::get_test_model()
{
    ModelARX xx({ -0.4, 0.2 }, { 0.6, 0.3 }, 2, 0.08);
    std::vector<double> x{ 0.1, 0.0, 0.5, 0, 2, -0.2, -0.1, 0.36 };
    for (const auto i : std::initializer_list<double>{ 0.1, 0.0, 0.5, 0, 2, -0.2, -0.1, 0.36 }) {
        xx.symuluj(i);
    }
    return xx;
}

void Testy_ModelARX::test_dump_eq()
{
    std::cerr << "Serialization and deserialization -> equivalence: ";
    auto xx = get_test_model();
    const auto dump = xx.dump();
    ModelARX restored{ dump };
    if (xx != restored)
        throw std::logic_error{ "Restored model does not compare equal" };
    for (const auto i : std::initializer_list<double>{ 0.3, -0.2, -0.1, 0, -0.3, -0, 0.1, 0.15 }) {
        if (xx.symuluj(i) != restored.symuluj(i))
            throw std::logic_error{ "Restored model behaves differently" };
    }
    if (xx.dump() != restored.dump())
        throw std::logic_error{ "Dumps do not compare equal" };
    std::cerr << "OK!\n";
}

void Testy_ModelARX::test_dump_length()
{
    std::cerr << "Serialization and deserialization -> total length check: ";
    auto xx = get_test_model();
    const auto dump = xx.dump();
    try {
        [[maybe_unused]] const auto _ = ModelARX{ dump.cbegin(), std::prev(dump.cend()) };
        std::cerr << "FAIL!\n";
        std::logic_error{ "Model can be restored from a buffer that is too short" };
    } catch (const std::runtime_error &) {
    }
    std::cerr << "OK!\n";
}

void Testy_ModelARX::test_dump_very_small()
{
    std::cerr << "Serialization and deserialization -> constant length part check: ";
    std::array data{ 0x0_u8, 0xFF_u8, 0xFF_u8, 0xDE_u8, 0xAD_u8, 0xBE_u8, 0xEF_u8 };
    try {
        [[maybe_unused]] const auto _ = ModelARX{ data.cbegin(), std::prev(data.cend()) };
        std::cerr << "FAIL!\n";
        std::logic_error{ "Model can be restored from a buffer that is way too short" };
    } catch (const std::runtime_error &) {
    }
    std::cerr << "OK!\n";
}

void Testy_ModelARX::test_dump_file()
{
    std::cerr << "Serialization and deserialization -> file dump: ";
    auto xx = get_test_model();
    const auto dump = xx.dump();
    std::fstream f{ "model.arx",
                    std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc };
    f.write(reinterpret_cast<const char *>(dump.data()), static_cast<int64_t>(dump.size()));
    f.flush();
    f.sync(); // just to make sure
    f.seekg(0, std::ios::end);
    const auto file_size = f.tellg();
    f.seekg(0, std::ios::beg);
    const auto buff = std::make_unique_for_overwrite<uint8_t[]>(static_cast<size_t>(file_size));
    f.read(reinterpret_cast<char *>(buff.get()), file_size);
    f.close();
    ModelARX restored{ buff.get(), buff.get() + file_size };
    std::cerr << (xx == restored ? "OK!\n" : "FAIL!\n");
}

void Testy_ModelARX::test_stream_op()
{
    std::cerr << "Stream operators -> equality check: ";
    std::stringstream buff;
    auto xx = get_test_model();
    buff << xx;
    ModelARX yy{ { 0 }, { 1 }, 3, 9.5 };
    buff >> yy;
    std::cerr << (xx == yy ? "OK!\n" : "FAIL!\n");
}

void Testy_ModelARX::run_tests()
{
    test_ModelARX_brakPobudzenia();
    test_ModelARX_skokJednostkowy_1();
    test_ModelARX_skokJednostkowy_2();
    test_ModelARX_skokJednostkowy_3();
    auto xx = get_test_model();
    std::cout << "delay: " << xx.m_transport_delay << " - just proving we're friends\n";
    test_dump_eq();
    test_dump_length();
    test_dump_very_small();
    test_dump_file();
    test_stream_op();
}

std::ostream &operator<<(std::ostream &os, const ModelARX &m)
{
    const auto precision = os.precision();
    os.precision(std::numeric_limits<double>::max_digits10);
    // Format similar to the one used in OI and competitive programming
    os << m.m_distribution.mean() << ' ' << m.m_distribution.stddev() << '\n'
       << m.m_init_seed << ' ' << m.m_n_generated << '\n'
       << m.m_coeff_a.size() << '\n';
    auto delim{ "" };
    for (const auto v : m.m_coeff_a) {
        os << delim << v;
        delim = " ";
    }
    os << '\n' << m.m_coeff_b.size() << '\n';
    delim = "";
    for (const auto v : m.m_coeff_b) {
        os << delim << v;
        delim = " ";
    }
    os << '\n' << m.m_in_signal_mem.size() << '\n';
    delim = "";
    for (const auto v : m.m_in_signal_mem) {
        os << delim << v;
        delim = " ";
    }
    os << '\n' << m.m_out_signal_mem.size() << '\n';
    delim = "";
    for (const auto v : m.m_out_signal_mem) {
        os << delim << v;
        delim = " ";
    }
    os << '\n' << m.m_delay_mem.size() << '\n';
    delim = "";
    for (const auto v : m.m_delay_mem) {
        os << delim << v;
        delim = " ";
    }
    os.precision(precision);
    return os << '\n';
}

std::istream &operator>>(std::istream &is, ModelARX &m)
{
    double dist_mean, dist_stddev;
    uint64_t seed, n_generated;
    is >> dist_mean >> dist_stddev >> seed >> n_generated;
    m.m_distribution = decltype(m.m_distribution){ dist_mean, dist_stddev };
    m.m_mt.seed(seed);
    m.m_init_seed = seed;
    m.m_n_generated = 0;
    while (m.m_n_generated < n_generated)
        m.get_random();
    const auto read_container = [&is](auto &container) -> uint64_t {
        uint64_t num;
        is >> num;
        container.resize(num);
        std::copy_n(std::istream_iterator<double>(is), num, container.begin());
        return num;
    };
    read_container(m.m_coeff_a);
    read_container(m.m_coeff_b);
    read_container(m.m_in_signal_mem);
    read_container(m.m_out_signal_mem);
    auto delay = read_container(m.m_delay_mem);
    m.m_transport_delay = static_cast<uint32_t>(delay);
    return is;
}
