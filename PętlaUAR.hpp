#pragma once
#include "ModelARX.h"
#include "ObiektSISO.h"
#include "ObiektStatyczny.hpp"
#include "RegulatorPID.h"
#include <cassert>
#include <memory>
#include <vector>

/// Control loop derived from ObiektSISO
class PętlaUAR : public ObiektSISO {
public:
    /// Unique name/prefix used to distinguish types in deserialization.
    constexpr static std::string_view unique_name{ "UAR" };

private:
    /// Size of the unique prefix.
    static constexpr std::size_t prefix_size{ unique_name.size()
                                              * sizeof(decltype(unique_name)::value_type) };

    /// Vector of the loop's components.
    std::vector<std::unique_ptr<ObiektSISO>> m_loop{};
    /// Whether the loop is closed and forms a feedback loop.
    bool m_closed;
    /// Stored previous simulation result.
    double m_prev_result;

    /// @brief Check if pointer is not `nullptr`.
    /// @param ptr reference to unique pointer to check
    /// @throws `std::runtime_error` if the pointer in `nullptr`
    constexpr static void check_ptr(const std::unique_ptr<ObiektSISO> &ptr)
    {
        if (ptr == nullptr)
            throw std::runtime_error{ "Inserted pointers must not be null" };
    }

public:
    /// @brief Regular constructor accepting basic loop parameters (closed and initial value);
    /// default constructor
    /// @param closed whether the loop is closed and should form a feedback loop
    /// @param init_val initial value of saved previous result; used to calculate initial feedback
    /// in closed loop
    constexpr PętlaUAR(bool closed = true, double init_val = 0.0)
        : m_closed{ closed }
        , m_prev_result{ init_val }
    {
    }
    /// @brief Deserializing constructor for an input range of bytes
    /// @tparam T type of the input range
    /// @param serialized input range over bytes representing serialized PętlaUAR
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
    /// @brief Reset the previous result and call @link ObiektSISO::reset() reset()@endlink on all
    /// components.
    /// @param init_val the new value of saved previous result
    void reset(double init_val);
    /// @brief Set the previous result to `0` and call @link ObiektSISO::reset() reset()@endlink on
    /// all components.
    void reset() override { reset(0.0); };
    /// @brief Simulate loop's response to the input signal.
    ///
    /// If the loop is closed (default) the first component receives the error signal, which is the
    /// difference between `u` and #m_prev_result. In open loop the input to the first component is
    /// simply `u`. Subsequent componets process outputs from their predecesors.<br>
    /// Output of the last component is saved as #m_prev_result and returned.
    ///
    /// Simulation uses ObiektSISO::symuluj() method of the components.
    ///
    /// @param u loop's input, the setpoint in closed loop
    /// @return response from the last loop component
    double symuluj(double u) override;
    /// Remove all componets.
    constexpr void clear() noexcept { m_loop.clear(); }
    /// @brief Get size of the loop.
    /// @return Number of components in the loop
    constexpr std::size_t size() const noexcept { return m_loop.size(); }
    /// Last result (#m_prev_result) getter.
    constexpr double get_last_result() const noexcept { return m_prev_result; }
    /// Closed setting (#m_closed) getter.
    constexpr bool get_closed() const noexcept { return m_closed; }
    /// Last result (#m_prev_result) setter.
    constexpr void set_init(double init_val) noexcept { m_prev_result = init_val; }
    /// Closed setting (#m_closed) setter.
    constexpr void set_closed(bool closed) noexcept { m_closed = closed; }
    /// @brief Append component to the loop.
    /// @param element the component to append at the back
    constexpr void push_back(std::unique_ptr<ObiektSISO> &&element)
    {
        check_ptr(element);
        m_loop.push_back(std::move(element));
    }
    /// @brief Append component to the loop and return its position.
    /// @param value the component to append at the back
    /// @return Index of the appended component (size of the loop - 1).
    constexpr std::size_t insert(std::unique_ptr<ObiektSISO> &&value)
    {
        push_back(std::move(value));
        return m_loop.size() - 1;
    }
    /// @brief Insert component at given index.
    /// @param index position at which the component should be inserted
    /// @param value the component to be inserted
    /// @return Index of the inserted component.
    constexpr std::size_t insert(std::size_t index, std::unique_ptr<ObiektSISO> &&value)
    {
        check_ptr(value);
        const auto it = std::next(m_loop.cbegin(), index);
        const auto oit = m_loop.insert(it, std::move(value));
        return static_cast<std::size_t>(std::distance(m_loop.begin(), oit));
    }
    /// @brief Insert multiple components starting at given index.
    /// @tparam ...Args type of components (must be convertible to std::unique_ptr<ObiektSISO>)
    /// @param index position at which the first of components should be inserted
    /// @param value first component to be inserted
    /// @param ...args remaining components to be inserted
    /// @return Index of the last inserted component.
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
    /// @brief Remove component at given index.
    /// @param index index of the element to be removed
    /// @return Index of the next element after erasing.
    /// @throws `std::range_error` if index is not less than the loop size
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

    /// @brief Overloaded equal comparison operator, which compares all parameters and checks if
    /// loop has the same types of components, which also compare equal.
    constexpr friend bool operator==(const PętlaUAR &a, const PętlaUAR &b)
    {
        if (a.m_closed != b.m_closed || a.m_prev_result != b.m_prev_result
            || a.m_loop.size() != b.m_loop.size())
            return false;
        for (std::size_t i = 0; i < a.m_loop.size(); ++i) {
            const ObiektSISO *const ap = a.m_loop[i].get();
            const ObiektSISO *const bp = b.m_loop[i].get();
            if (const auto a_pid = dynamic_cast<const RegulatorPID *>(ap)) {
                const auto b_pid = dynamic_cast<const RegulatorPID *>(bp);
                if (b_pid == nullptr || *a_pid != *b_pid)
                    return false;
            } else if (const auto a_static = dynamic_cast<const ObiektStatyczny *>(ap)) {
                const auto b_static = dynamic_cast<const ObiektStatyczny *>(bp);
                if (b_static == nullptr || *a_static != *b_static)
                    return false;
            } else if (const auto a_arx = dynamic_cast<const ModelARX *>(ap)) {
                const auto b_arx = dynamic_cast<const ModelARX *>(bp);
                if (b_arx == nullptr || *a_arx != *b_arx)
                    return false;
            } else if (const auto a_uar = dynamic_cast<const PętlaUAR *>(ap)) {
                const auto b_uar = dynamic_cast<const PętlaUAR *>(bp);
                if (b_uar == nullptr || *a_uar != *b_uar)
                    return false;
            } else {
                assert(false);
            }
        }
        return true;
    }
    friend class TreeModel;
};
DESERIALIZABLE_SISO(PętlaUAR);

#ifdef LAB_TESTS
class UARTests {
private:
    static void test_simple_pid_arx();
    static void test_uar_serialization();

public:
    static void run_tests();
};
#endif
