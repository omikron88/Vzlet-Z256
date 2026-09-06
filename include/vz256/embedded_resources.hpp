#pragma once

#include <cstdint>
#include <span>

namespace vz256 {

[[nodiscard]] std::span<const std::uint8_t> embedded_monitor_rom();
[[nodiscard]] std::span<const std::uint8_t> embedded_character_rom();
[[nodiscard]] std::span<const std::uint8_t> embedded_boot_disk();

} // namespace vz256
