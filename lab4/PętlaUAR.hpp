#pragma once
#include "../lab1/ObiektSISO.h"
#include <memory>
#include <vector>

class PętlaUAR : public ObiektSISO {
public:
    constexpr static std::string_view unique_name{ "UAR" };

private:
    static constexpr std::size_t prefix_size{ unique_name.size()
                                              * sizeof(decltype(unique_name)::value_type) };

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
    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    PętlaUAR(const T &serialized)
    {
        constexpr std::size_t min_size = sizeof(uint32_t) + prefix_size + sizeof(uint8_t)
            + (sizeof m_prev_result) + sizeof(uint64_t);
        const auto s = std::ranges::size(serialized);
        if (s < min_size)
            throw std::runtime_error{ "Data size is smaller than the minimum size" };

        const auto data_len = from_byte_range<uint32_t>(serialized);
        if (s < data_len + sizeof(uint32_t))
            throw std::runtime_error{ "Data size is smaller than expected" };

        std::size_t drop_c = sizeof(uint32_t);
        if (!prefix_match(unique_name, serialized | std::views::drop(drop_c)))
            throw std::runtime_error{
                "PętlaUAR serialized data does not start with the expected prefix"
            };

        drop_c += prefix_size;
        m_closed = (*std::ranges::begin(serialized | std::views::drop(drop_c))) > 0_u8;
        drop_c += 1;
        m_prev_result = from_byte_range<double>(serialized | std::views::drop(drop_c));
        drop_c += sizeof(double);
        const auto n_elements = from_byte_range<uint64_t>(serialized | std::views::drop(drop_c));
        drop_c += sizeof(uint64_t);
        m_loop.reserve(n_elements);
        for (std::uint64_t i = 0; i < n_elements; i++) {
            const auto l = from_byte_range<uint32_t>(serialized | std::views::drop(drop_c));
            auto e = ObiektSISO::deserialize(serialized | std::views::drop(drop_c)
                                             | std::views::take(l + sizeof(uint32_t)));
            m_loop.push_back(std::move(e));
            drop_c += sizeof(uint32_t) + l;
        }
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
    constexpr std::vector<uint8_t> dump() const override
    {
        const auto prefix = range_to_bytes(unique_name);
        const auto closed = to_bytes(m_closed ? 1_u8 : 0_u8);
        const auto last = to_bytes(m_prev_result);
        const auto n_elements = to_bytes(static_cast<uint64_t>(m_loop.size()));
        const auto elements = m_loop
            | std::views::transform([](const auto &e) { return e->dump(); }) | std::views::join
            | std::ranges::to<std::vector<uint8_t>>();
        const auto len_b = to_bytes(static_cast<uint32_t>(prefix_size + closed.size() + last.size()
                                                          + n_elements.size() + elements.size()));
        return concat_iterables(len_b, prefix, closed, last, n_elements, elements);
    }
};
DESERIALIZABLE_SISO(PętlaUAR);
