#include <cstdint>
#include <cmath>
#include <stdexcept>

constexpr uint8_t operator"" _u8(unsigned long long a) noexcept {
	return static_cast<uint8_t>(a);
}

constexpr inline bool is_bad_or_neg(double x) {
	return std::signbit(x) || !std::isfinite(x);
}

constexpr inline void throw_bad_neg(double x) {
	if (is_bad_or_neg(x)) throw std::runtime_error{"parameter must be nonnegative and finite"};
}
