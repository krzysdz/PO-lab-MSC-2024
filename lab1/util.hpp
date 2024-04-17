#include <cstdint>

constexpr uint8_t operator"" _u8(unsigned long long a) noexcept {
	return static_cast<uint8_t>(a);
}
