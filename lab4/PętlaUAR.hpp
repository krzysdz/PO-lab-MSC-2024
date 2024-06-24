#pragma once
#include "../lab1/ObiektSISO.h"
#include <memory>
#include <vector>

class PętlaUAR : public ObiektSISO {
private:
    std::vector<std::unique_ptr<ObiektSISO>> m_loop{};
    bool m_closed;
    double m_prev_result;

    constexpr static void check_ptr(const std::unique_ptr<ObiektSISO> &ptr)
    {
        if (ptr == nullptr)
            throw std::runtime_error{ "Inserted pointers must not be null" };
    }

public:
    constexpr PętlaUAR(bool closed = true, double init_val = 0.0)
        : m_closed{ closed }
        , m_prev_result{ init_val }
    {
    }
    void reset(double init_val);
    void reset() override { reset(0.0); };
    double symuluj(double u) override;
    constexpr void clear() noexcept { m_loop.clear(); }
    constexpr std::size_t size() const noexcept { return m_loop.size(); }
    constexpr void set_init(double init_val) noexcept { m_prev_result = init_val; }
    constexpr void set_closed(bool closed) noexcept { m_closed = closed; }
    constexpr void push_back(std::unique_ptr<ObiektSISO> &&element)
    {
        check_ptr(element);
        m_loop.push_back(std::move(element));
    }
    constexpr std::size_t insert(std::unique_ptr<ObiektSISO> &&value)
    {
        push_back(std::move(value));
        return m_loop.size() - 1;
    }
    constexpr std::size_t insert(std::size_t index, std::unique_ptr<ObiektSISO> &&value)
    {
        check_ptr(value);
        const auto it = std::next(m_loop.cbegin(), index);
        const auto oit = m_loop.insert(it, std::move(value));
        return static_cast<std::size_t>(std::distance(m_loop.begin(), oit));
    }
    template <typename... Args>
    constexpr std::size_t insert(std::size_t index, std::unique_ptr<ObiektSISO> &&value,
                                 Args &&...args)
    {
        const auto ins_idx = insert(index, std::move(value));
        if constexpr (sizeof...(Args)) {
            insert(ins_idx + 1, std::forward<Args>(args)...);
        }
        return ins_idx;
    }
    constexpr std::size_t erase(std::size_t index)
    {
        if (index >= m_loop.size())
            throw std::range_error{ "Index out of range" };
        const auto it = std::next(m_loop.cbegin(), index);
        const auto oit = m_loop.erase(it);
        return static_cast<std::size_t>(std::distance(m_loop.begin(), oit));
    }
};
