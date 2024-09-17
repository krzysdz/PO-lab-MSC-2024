/// @file ObiektSISO.h

#pragma once
#include "define_fixes.hpp"
#include "util.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class ObiektSISO;

/// Vector of [prefix, deserializer function] pairs.
extern std::vector<
    std::pair<std::vector<uint8_t>, std::unique_ptr<ObiektSISO> (*)(const std::vector<uint8_t> &)>>
    siso_deserializers;
/// @brief Declare class @a class_name as deserializable and add its deserializer to
/// #siso_deserializers.
/// @details The class must have a `::unique_name` member.
#define DESERIALIZABLE_SISO(class_name)                                                            \
    namespace {                                                                                    \
        [[maybe_unused]] const auto __add_serializable_generator__##class_name                     \
            = siso_deserializers.emplace_back(                                                     \
                std::ranges::to<std::vector<uint8_t>>(range_to_bytes(class_name::unique_name)),    \
                [](const std::vector<uint8_t> &bin_data) -> std::unique_ptr<ObiektSISO> {          \
                    return std::make_unique<class_name>(bin_data);                                 \
                });                                                                                \
    }

/// Abstract class describing an object with single input and single output.
class ObiektSISO {
public:
    /// Reset the object, noop if not overriden.
    virtual void reset() { }
    /// @brief Perform a simulation.
    /// @param u simulation input
    /// @return simulation output
    virtual double symuluj(double u) = 0;
    /// @brief Serialize the object.
    /// @return A vector of bytes (`uint8_t`) from which the object can be reconstructed.
    virtual std::vector<uint8_t> dump() const = 0;
    virtual ~ObiektSISO() = default;

    /// @brief Deserialize `serialized` into an ObiektSISO derived class based on data prefix.
    ///
    /// The serialized (target) class must be registered using #DESERIALIZABLE_SISO.
    ///
    /// @tparam T type of input range over bytes
    /// @param serialized byte representation of an ObiektSISO derived class
    /// @return A unique pointer owning a deserialized instance of an appropriate class
    /// @throws `std::runtime_error` if the data does not match any registered class.
    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    static std::unique_ptr<ObiektSISO> deserialize(const T &serialized)
    {
        for (const auto &[name, factory] : siso_deserializers) {
#if __cpp_lib_ranges_starts_ends_with >= 202106L
            if (std::ranges::starts_with(serialized | std::views::drop(sizeof(uint32_t)), name))
#else
            if (std::ranges::mismatch(serialized | std::views::drop(sizeof(uint32_t)), name).in2
                == std::ranges::end(name))
#endif
                return factory(serialized | std::ranges::to<std::vector<uint8_t>>());
        }
        throw std::runtime_error{ "Serialized data does not match any known object." };
    }

    friend bool operator==(const ObiektSISO &, const ObiektSISO &) = default;
    friend bool operator!=(const ObiektSISO &, const ObiektSISO &) = default;
};

#ifdef LAB_TESTS
class TESTY {
public:
    static void raportBleduSekwencji(std::vector<double> &spodz, std::vector<double> &fakt);
    static bool porownanieSekwencji(std::vector<double> &spodz, std::vector<double> &fakt);
};
#endif
