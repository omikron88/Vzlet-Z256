#include "vz256/video.hpp"

#include <algorithm>

namespace vz256 {

std::uint8_t Video::read(std::uint8_t bank, std::uint16_t address) const {
    switch (bank & 3U) {
    case 0: return chars_[address % chars_.size()];
    case 1: return high_[address];
    case 2: return low_[address];
    default: return (address & 1U) == 0 ? crtc_index_ : crtc_[crtc_index_ & 31U];
    }
}

void Video::write(std::uint8_t bank, std::uint16_t address, std::uint8_t value) {
    switch (bank & 3U) {
    case 1: high_[address] = value; break;
    case 2: low_[address] = value; break;
    case 3:
        if ((address & 1U) == 0) crtc_index_ = static_cast<std::uint8_t>(value & 31U);
        else crtc_[crtc_index_ & 31U] = value;
        break;
    default: break; // character EPROM is read-only
    }
}

void Video::set_character_rom(std::span<const std::uint8_t> data) {
    if (data.empty()) return;
    for (std::size_t i = 0; i < chars_.size(); ++i) chars_[i] = data[i % data.size()];
}

void Video::render(std::span<std::uint32_t> rgba) const {
    if (rgba.size() < static_cast<std::size_t>(width * height)) return;
    constexpr std::array<std::uint8_t, 4> levels{0, 85, 170, 255};
    for (int y = 0; y < height; ++y) {
        for (int byte_x = 0; byte_x < width / 8; ++byte_x) {
            const auto address = static_cast<std::uint16_t>(0x8000 + y * (width / 8) + byte_x);
            for (int bit = 0; bit < 8; ++bit) {
                const auto mask = static_cast<std::uint8_t>(0x80U >> bit);
                const unsigned color = ((high_[address] & mask) ? 2U : 0U) |
                                       ((low_[address] & mask) ? 1U : 0U);
                const std::uint32_t gray = levels[color];
                rgba[static_cast<std::size_t>(y * width + byte_x * 8 + bit)] =
                    0xFF000000U | (gray << 16U) | (gray << 8U) | gray;
            }
        }
    }
}

void Video::reset() {
    crtc_.fill(0);
    crtc_index_ = 0;
}

} // namespace vz256
