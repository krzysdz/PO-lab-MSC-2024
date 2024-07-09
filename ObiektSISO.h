#pragma once
#include "define_fixes.hpp"
#include "util.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class ObiektSISO;

extern std::vector<
    std::pair<std::vector<uint8_t>, std::unique_ptr<ObiektSISO> (*)(const std::vector<uint8_t> &)>>
    siso_serializers;
#define DESERIALIZABLE_SISO(class_name)                                                            \
    namespace {                                                                                    \
        [[maybe_unused]] const auto __add_serializable_generator__##class_name                     \
            = siso_serializers.emplace_back(                                                       \
                std::ranges::to<std::vector<uint8_t>>(range_to_bytes(class_name::unique_name)),    \
                [](const std::vector<uint8_t> &bin_data) -> std::unique_ptr<ObiektSISO> {          \
                    return std::make_unique<class_name>(bin_data);                                 \
                });                                                                                \
    }

class ObiektSISO {
public:
    virtual void reset() { }
    virtual double symuluj(double u) = 0;
    virtual std::vector<uint8_t> dump() const = 0;
    virtual ~ObiektSISO() = default;

    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    static std::unique_ptr<ObiektSISO> deserialize(const T &serialized)
    {
        for (const auto &[name, factory] : siso_serializers) {
#if __cpp_lib_ranges_starts_ends_with >= 202106L
            if (std::ranges::starts_with(serialized, name))
#else
            if (std::ranges::mismatch(serialized, name).in2 == std::ranges::end(name))
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
