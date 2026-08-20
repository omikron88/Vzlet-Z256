#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace vz256 {

class Video {
public:
    static constexpr int width = 640;
    static constexpr int height = 300;
    static constexpr std::size_t plane_size = 65536;

    std::uint8_t read(std::uint8_t bank, std::uint16_t address) const;
    void write(std::uint8_t bank, std::uint16_t address, std::uint8_t value);
    void set_character_rom(std::span<const std::uint8_t> data);
    void render(std::span<std::uint32_t> rgba) const;
    void reset();

private:
    [[nodiscard]] static std::uint16_t display_address(std::uint16_t ma,
                                                       std::uint8_t raster);

    std::array<std::uint8_t, plane_size> high_{};
    std::array<std::uint8_t, plane_size> low_{};
    std::array<std::uint8_t, 8192> chars_{};
    std::array<std::uint8_t, 32> crtc_{};
    std::uint8_t crtc_index_{};
};

} // namespace vz256
