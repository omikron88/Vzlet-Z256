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

std::uint16_t Video::display_address(std::uint16_t ma, std::uint8_t raster) {
    // The video board does not store scan lines consecutively. MC6845 MA0..7
    // drive RAM A0..7, RA0..3 drive A8..11 and MA8..11 drive A12..15. The
    // displayed half of VRAM is selected by forcing the top address bit high.
    return static_cast<std::uint16_t>(
        0x8000U | ((ma & 0x0F00U) << 4U) |
        (static_cast<std::uint16_t>(raster & 0x0FU) << 8U) | (ma & 0x00FFU));
}

void Video::render(std::span<std::uint32_t> rgba) const {
    if (rgba.size() < static_cast<std::size_t>(width * height)) return;
    constexpr std::array<std::uint8_t, 4> levels{0, 85, 170, 255};
    std::fill_n(rgba.begin(), static_cast<std::size_t>(width * height), 0xFF000000U);

    const auto columns = std::min<unsigned>(crtc_[1], width / 8);
    const auto rows = static_cast<unsigned>(crtc_[6]);
    const auto scanlines = std::min<unsigned>((crtc_[9] & 0x1FU) + 1U, 16U);
    const auto active_height = std::min<unsigned>(rows * scanlines, height);
    const auto start = static_cast<std::uint16_t>(
        ((static_cast<std::uint16_t>(crtc_[12] & 0x3FU) << 8U) | crtc_[13]) & 0x3FFFU);

    for (unsigned y = 0; y < active_height; ++y) {
        const auto character_row = y / scanlines;
        const auto raster = static_cast<std::uint8_t>(y % scanlines);
        const auto row_ma = static_cast<std::uint16_t>(
            (start + character_row * columns) & 0x3FFFU);
        for (unsigned byte_x = 0; byte_x < columns; ++byte_x) {
            const auto address = display_address(
                static_cast<std::uint16_t>((row_ma + byte_x) & 0x3FFFU), raster);
            for (int bit = 0; bit < 8; ++bit) {
                const auto mask = static_cast<std::uint8_t>(0x80U >> bit);
                const unsigned color = ((high_[address] & mask) ? 2U : 0U) |
                                       ((low_[address] & mask) ? 1U : 0U);
                const std::uint32_t gray = levels[color];
                rgba[static_cast<std::size_t>(y * width + byte_x * 8U + static_cast<unsigned>(bit))] =
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
