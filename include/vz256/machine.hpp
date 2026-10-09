#pragma once

#include "vz256/floppy.hpp"
#include "vz256/ctc.hpp"
#include "vz256/pio.hpp"
#include "vz256/sio.hpp"
#include "vz256/video.hpp"
#include "vz256/wd2797.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <deque>
#include <span>
#include <vector>

namespace vz256 {

class Machine {
public:
    static constexpr std::uint32_t cpu_hz = 4'000'000;

    struct FloppyStatus {
        std::uint8_t drive{};
        std::uint8_t track{};
        std::uint8_t side{};
        std::uint8_t sector{};
        Wd2797::Activity activity{Wd2797::Activity::idle};
        bool drq{};
        bool intrq{};
    };

    bool load_roms(const std::filesystem::path& monitor,
                   const std::filesystem::path& characters);
    bool load_roms(std::span<const std::uint8_t> monitor,
                   std::span<const std::uint8_t> characters);
    void reset();

    std::uint8_t read(std::uint16_t address, bool opcode = false) const;
    void write(std::uint16_t address, std::uint8_t value);
    std::uint8_t input(std::uint16_t port);
    void output(std::uint16_t port, std::uint8_t value);
    void key(std::uint8_t ascii);
    void tick(std::uint32_t cycles);
    [[nodiscard]] bool interrupt_pending() const;
    [[nodiscard]] std::uint8_t interrupt_vector();
    [[nodiscard]] std::vector<std::uint8_t> take_printer_output();
    void serial_receive(std::uint8_t channel, std::uint8_t value);
    [[nodiscard]] std::vector<std::uint8_t> take_serial_output(std::uint8_t channel);
    void fdc_serial_receive(std::uint8_t channel, std::uint8_t value);
    [[nodiscard]] std::vector<std::uint8_t> take_fdc_serial_output(std::uint8_t channel);

    [[nodiscard]] Video& video() { return video_; }
    [[nodiscard]] FloppyImage& drive(std::size_t index) { return drives_.at(index); }
    [[nodiscard]] bool media_change_allowed() const { return fdc_.idle(); }
    [[nodiscard]] FloppyStatus floppy_status() const;

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
    std::vector<std::uint8_t> printer_output_;
    Pio cpu_pio_;
    Pio fdc_pio_;
    Ctc cpu_ctc_;
    Sio cpu_sio_;
    Ctc fdc_ctc_;
    Sio fdc_sio_;
    std::uint8_t fdc_ctc_clock_phase_{};
};

} // namespace vz256
