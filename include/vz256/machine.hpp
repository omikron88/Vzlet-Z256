#pragma once

#include "vz256/floppy.hpp"
#include "vz256/video.hpp"
#include "vz256/wd2797.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <deque>
#include <span>

namespace vz256 {

class Machine {
public:
    static constexpr std::uint32_t cpu_hz = 4'000'000;

    bool load_roms(const std::filesystem::path& monitor,
                   const std::filesystem::path& characters);
    void reset();

    std::uint8_t read(std::uint16_t address, bool opcode = false) const;
    void write(std::uint16_t address, std::uint8_t value);
    std::uint8_t input(std::uint16_t port);
    void output(std::uint16_t port, std::uint8_t value);
    void key(std::uint8_t ascii);
    void tick(std::uint32_t cycles) { fdc_.tick(cycles); }
    [[nodiscard]] bool interrupt_pending() const;
    [[nodiscard]] std::uint8_t interrupt_vector() const;

    [[nodiscard]] Video& video() { return video_; }
    [[nodiscard]] FloppyImage& drive(std::size_t index) { return drives_.at(index); }
    [[nodiscard]] bool media_change_allowed() const { return fdc_.idle(); }

private:
    [[nodiscard]] std::uint8_t page_for_read(bool opcode) const;
    [[nodiscard]] bool monitor_at(std::uint16_t address) const;

    std::array<std::array<std::uint8_t, 65536>, 4> ram_{};
    std::array<std::uint8_t, 8192> monitor_rom_{};
    std::array<std::uint8_t, 8192> monitor_ram_{};
    Video video_;
    std::array<FloppyImage, 4> drives_;
    Wd2797 fdc_;
    std::uint8_t paging_{};
    std::uint8_t secondary_{};
    std::deque<std::uint8_t> keyboard_queue_;
    bool pio_a_interrupt_enabled_{};
    std::uint8_t pio_b_output_{};
    std::uint8_t pio_b_vector_{};
    bool pio_b_interrupt_enabled_{};
    bool pio_b_expect_direction_{};
    bool pio_b_expect_interrupt_mask_{};
    bool motor_timer_phase_{};
};

} // namespace vz256
