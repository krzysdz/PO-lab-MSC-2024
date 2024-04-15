#pragma once
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <random>
#include <stdexcept>
#include <vector>

#include "ObiektSISO.hpp"

template <std::floating_point T = double, std::unsigned_integral DT = std::uint32_t>
class ModelARX : public ObiektSISO<T> {
private:
    std::vector<T> m_coeff_a;
    std::vector<T> m_coeff_b;
    DT m_transport_delay;
    std::normal_distribution<T> m_distribution;
    std::deque<T> m_in_signal_mem;
    std::deque<T> m_out_signal_mem;
    std::deque<T> m_delay_mem;
    std::mt19937_64 m_mt;

public:
    ModelARX() = delete;
    ModelARX(std::vector<T> &&coeff_a, std::vector<T> &&coeff_b, const DT delay = 1,
             const T stddev = 0.0);
    template <std::integral DLT, typename DEVT>
        requires std::is_convertible_v<DLT, DT> && std::is_convertible_v<DEVT, T>
    ModelARX(std::vector<T> &&coeff_a, std::vector<T> &&coeff_b, const DLT delay = 1,
             const DEVT stddev = 0.0)
        : ModelARX{ std::move(coeff_a), std::move(coeff_b), static_cast<DT>(delay),
                    static_cast<T>(stddev) }
    {
        if constexpr (std::is_signed_v<DLT>) {
            if (delay < DLT{})
                throw std::runtime_error("Delay must be >= 0");
        }
    }
    void set_coeff_a(std::vector<T> &&coefficients) noexcept;
    void set_coeff_b(std::vector<T> &&coefficients) noexcept;
    void set_transport_delay(const DT delay);
    void set_stddev(const T stddev);
    T symuluj(T u) override;
    ~ModelARX() = default;
};

#include "ModelARX.cpp"
