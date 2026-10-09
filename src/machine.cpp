#include "vz256/machine.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace vz256 {
namespace {

bool read_file(const std::filesystem::path& path, std::span<std::uint8_t> destination) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    const std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(stream), {}};
    if (data.empty() || data.size() > destination.size()) return false;
    for (std::size_t i = 0; i < destination.size(); ++i) {
        destination[i] = data[i % data.size()];
    }
    return true;
}

bool copy_mirrored(std::span<const std::uint8_t> source,
                   std::span<std::uint8_t> destination) {
    if (source.empty() || source.size() > destination.size()) return false;
    for (std::size_t i = 0; i < destination.size(); ++i) {
        destination[i] = source[i % source.size()];
    }
    return true;
}

} // namespace

bool Machine::load_roms(const std::filesystem::path& monitor,
                        const std::filesystem::path& characters) {
    return read_file(monitor, monitor_rom_) &&
           ([&] {
               std::array<std::uint8_t, 8192> data{};
               if (!read_file(characters, data)) return false;
               video_.set_character_rom(data);
               return true;
           })();
}

bool Machine::load_roms(std::span<const std::uint8_t> monitor,
                        std::span<const std::uint8_t> characters) {
    std::array<std::uint8_t, 8192> character_data{};
    if (!copy_mirrored(monitor, monitor_rom_) ||
        !copy_mirrored(characters, character_data)) return false;
    video_.set_character_rom(character_data);
    return true;
}

void Machine::reset() {
    paging_ = 0;
    secondary_ = 0;
    keyboard_queue_.clear();
    cpu_pio_.reset();
    fdc_pio_.reset();
    cpu_pio_.set_input(0, 0x04); // printer ready: BUSY=0, PE=0, /ERROR=1
    cpu_pio_.write(0, 0x80);     // inactive active-low Centronics STROBE
    cpu_ctc_.reset();
    cpu_sio_.reset();
    fdc_ctc_.reset();
    fdc_sio_.reset();
    fdc_ctc_clock_phase_ = 0;
    fdc_.reset();
    video_.reset();
}

void Machine::tick(std::uint32_t cycles) {
    const auto selected_drive = static_cast<std::uint8_t>(fdc_pio_.output(1) & 3U);
    fdc_.tick(cycles, drives_, selected_drive);
    fdc_pio_.set_input(1, static_cast<std::uint8_t>((fdc_.intrq() ? 0x40U : 0U) |
                                                    (fdc_.drq() ? 0x80U : 0U)));
    cpu_ctc_.tick(cycles);
    // The FDC-board CTC receives a 1 MHz clock derived from the 4 MHz CPU
    // clock. Preserve the remainder so small instruction slices do not lose
    // input ticks.
    const auto fdc_clock_cycles = static_cast<std::uint32_t>(fdc_ctc_clock_phase_) + cycles;
    fdc_ctc_.tick(fdc_clock_cycles / 4U);
    fdc_ctc_clock_phase_ = static_cast<std::uint8_t>(fdc_clock_cycles % 4U);
    video_.tick(cycles);
}

std::uint8_t Machine::page_for_read(bool opcode) const {
    return static_cast<std::uint8_t>((paging_ >> (opcode ? 4U : 0U)) & 3U);
}

bool Machine::monitor_at(std::uint16_t address) const {
    return (paging_ & 0x80U) == 0 && address < 0x4000;
}

std::uint8_t Machine::read(std::uint16_t address, bool opcode) const {
    if (monitor_at(address)) {
        return address < 0x2000 ? monitor_rom_[address] : monitor_ram_[address - 0x2000];
    }
    const auto page = page_for_read(opcode);
    return (secondary_ & 1U) != 0 ? video_.read(page, address) : ram_[page][address];
}

void Machine::write(std::uint16_t address, std::uint8_t value) {
    if (monitor_at(address)) {
        if (address >= 0x2000) monitor_ram_[address - 0x2000] = value;
        return;
    }
    const auto page = static_cast<std::uint8_t>((paging_ >> 2U) & 3U);
    if ((secondary_ & 1U) != 0) video_.write(page, address, value);
    else ram_[page][address] = value;
}

std::uint8_t Machine::input(std::uint16_t port) {
    const auto p = static_cast<std::uint8_t>(port);
    if (p == 0xF0 || p == 0xF1)
        return cpu_pio_.read(static_cast<std::uint8_t>(p - 0xF0));
    if (p == 0xF8 || p == 0xF9)
        return cpu_sio_.read_data(static_cast<std::uint8_t>(p - 0xF8));
    if (p == 0xFA || p == 0xFB)
        return cpu_sio_.read_control(static_cast<std::uint8_t>(p - 0xFA));
    if (p >= 0xD0 && p <= 0xD3) {
        const auto value = fdc_.read(static_cast<std::uint8_t>(p - 0xD0), drives_,
                                     static_cast<std::uint8_t>(fdc_pio_.output(1) & 3U));
        fdc_pio_.set_input(1, static_cast<std::uint8_t>((fdc_.intrq() ? 0x40U : 0U) |
                                                        (fdc_.drq() ? 0x80U : 0U)));
        return value;
    }
    if (p == 0xD4) {
        if (keyboard_queue_.empty()) return 0xFF;
        fdc_pio_.set_input(0, static_cast<std::uint8_t>(~keyboard_queue_.front()));
        const auto value = fdc_pio_.read(0);
        keyboard_queue_.pop_front();
        if (!keyboard_queue_.empty()) {
            fdc_pio_.set_input(0, static_cast<std::uint8_t>(~keyboard_queue_.front()));
            fdc_pio_.strobe(0);
        }
        return value;
    }
    if (p == 0xF6) {
        return cpu_ctc_.read(2);
    }
    if (p >= 0xD8 && p <= 0xDB)
        return fdc_ctc_.read(static_cast<std::uint8_t>(p - 0xD8));
    if (p == 0xDC || p == 0xDE)
        return fdc_sio_.read_data(static_cast<std::uint8_t>((p - 0xDC) / 2U));
    if (p == 0xDD || p == 0xDF)
        return fdc_sio_.read_control(static_cast<std::uint8_t>((p - 0xDD) / 2U));
    if (p == 0xD5) {
        fdc_pio_.set_input(1, static_cast<std::uint8_t>((fdc_.intrq() ? 0x40U : 0U) |
                                                        (fdc_.drq() ? 0x80U : 0U)));
        return fdc_pio_.read(1);
    }
    return 0xFF;
}

void Machine::output(std::uint16_t port, std::uint8_t value) {
    const auto p = static_cast<std::uint8_t>(port);
    if (p == 0xF0) {
        // Centronics accepts the byte on the active-low STROBE edge. Holding
        // STROBE low must not emit the same byte repeatedly.
        const auto previous = cpu_pio_.output(0);
        cpu_pio_.write(0, value);
        if ((previous & 0x80U) != 0 && (value & 0x80U) == 0)
            printer_output_.push_back(cpu_pio_.output(1));
    } else if (p == 0xF1) {
        cpu_pio_.write(1, value);
    } else if (p == 0xF2 || p == 0xF3) {
        cpu_pio_.control(static_cast<std::uint8_t>(p - 0xF2), value);
    } else if (p == 0xF8 || p == 0xF9) {
        cpu_sio_.write_data(static_cast<std::uint8_t>(p - 0xF8), value);
    } else if (p == 0xFA || p == 0xFB) {
        cpu_sio_.write_control(static_cast<std::uint8_t>(p - 0xFA), value);
    } else if (p >= 0xD0 && p <= 0xD3) {
        fdc_.write(static_cast<std::uint8_t>(p - 0xD0), value, drives_,
                   static_cast<std::uint8_t>(fdc_pio_.output(1) & 3U));
        fdc_pio_.set_input(1, static_cast<std::uint8_t>((fdc_.intrq() ? 0x40U : 0U) |
                                                        (fdc_.drq() ? 0x80U : 0U)));
    } else if (p == 0xD5) {
        fdc_pio_.write(1, value);
    } else if (p == 0xD6) {
        fdc_pio_.control(0, value);
    } else if (p == 0xD7) {
        fdc_pio_.control(1, value);
        // The monitor uses adjacent IM2 entries for B and keyboard channel A.
        if ((value & 1U) == 0) fdc_pio_.control(0, static_cast<std::uint8_t>(value + 2U));
    } else if (p >= 0xF4 && p <= 0xF7) {
        cpu_ctc_.write(static_cast<std::uint8_t>(p - 0xF4), value);
    } else if (p >= 0xD8 && p <= 0xDB) {
        fdc_ctc_.write(static_cast<std::uint8_t>(p - 0xD8), value);
    } else if (p == 0xDC || p == 0xDE) {
        fdc_sio_.write_data(static_cast<std::uint8_t>((p - 0xDC) / 2U), value);
    } else if (p == 0xDD || p == 0xDF) {
        fdc_sio_.write_control(static_cast<std::uint8_t>((p - 0xDD) / 2U), value);
    } else if (p == 0xFC) paging_ = value;
    else if ((p & 0xF0U) == 0xC0U) secondary_ = static_cast<std::uint8_t>(p & 0x0FU);
    // Remaining devices deliberately return benign values until their timing
    // models are introduced; decoding them here keeps the bus contract stable.
}

std::vector<std::uint8_t> Machine::take_printer_output() {
    auto output = std::move(printer_output_);
    printer_output_.clear();
    return output;
}

void Machine::serial_receive(std::uint8_t channel, std::uint8_t value) {
    cpu_sio_.receive(channel, value);
}

std::vector<std::uint8_t> Machine::take_serial_output(std::uint8_t channel) {
    return cpu_sio_.take_transmitted(channel);
}

void Machine::fdc_serial_receive(std::uint8_t channel, std::uint8_t value) {
    fdc_sio_.receive(channel, value);
}

std::vector<std::uint8_t> Machine::take_fdc_serial_output(std::uint8_t channel) {
    return fdc_sio_.take_transmitted(channel);
}

bool Machine::interrupt_pending() const {
    return cpu_sio_.interrupt_pending() || cpu_ctc_.interrupt_pending() ||
           cpu_pio_.interrupt_pending() || fdc_pio_.interrupt_pending() ||
           fdc_ctc_.interrupt_pending() || fdc_sio_.interrupt_pending();
}

std::uint8_t Machine::interrupt_vector() {
    if (cpu_sio_.interrupt_pending()) return cpu_sio_.interrupt_acknowledge();
    if (cpu_ctc_.interrupt_pending()) return cpu_ctc_.interrupt_acknowledge();
    if (cpu_pio_.interrupt_pending()) return cpu_pio_.interrupt_acknowledge();
    if (fdc_pio_.interrupt_pending()) return fdc_pio_.interrupt_acknowledge();
    if (fdc_ctc_.interrupt_pending()) return fdc_ctc_.interrupt_acknowledge();
    if (fdc_sio_.interrupt_pending()) return fdc_sio_.interrupt_acknowledge();
    return 0xff;
}

void Machine::key(std::uint8_t ascii) {
    constexpr std::size_t keyboard_buffer_size = 16;
    if (keyboard_queue_.size() >= keyboard_buffer_size) return;
    const bool was_empty = keyboard_queue_.empty();
    keyboard_queue_.push_back(ascii);
    if (was_empty) {
        fdc_pio_.set_input(0, static_cast<std::uint8_t>(~ascii));
        fdc_pio_.strobe(0);
    }
}

Machine::FloppyStatus Machine::floppy_status() const {
    return FloppyStatus{static_cast<std::uint8_t>(fdc_pio_.output(1) & 3U),
                        fdc_.track(), fdc_.side(), fdc_.sector(), fdc_.activity(),
                        fdc_.drq(), fdc_.intrq()};
}

} // namespace vz256
