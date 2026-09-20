#include "vz256/machine.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
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
    pio_a_interrupt_enabled_ = false;
    pio_b_output_ = 0;
    pio_b_vector_ = 0;
    pio_b_interrupt_enabled_ = false;
    pio_b_expect_direction_ = false;
    pio_b_expect_interrupt_mask_ = false;
    cpu_ctc_.reset();
    fdc_.reset();
    video_.reset();
}

void Machine::tick(std::uint32_t cycles) {
    fdc_.tick(cycles);
    cpu_ctc_.tick(cycles);
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
    if (p >= 0xD0 && p <= 0xD3)
        return fdc_.read(static_cast<std::uint8_t>(p - 0xD0), drives_,
                         static_cast<std::uint8_t>(pio_b_output_ & 3U));
    if (p == 0xD4) {
        if (keyboard_queue_.empty()) return 0xFF;
        const auto value = keyboard_queue_.front();
        keyboard_queue_.pop_front();
        return static_cast<std::uint8_t>(~value); // keyboard data bus is active-low
    }
    if (p == 0xF6) {
        return cpu_ctc_.read(2);
    }
    if (p == 0xD5) {
        return static_cast<std::uint8_t>((pio_b_output_ & 0x3FU) |
                                        (fdc_.intrq() ? 0x40U : 0U) |
                                        (fdc_.drq() ? 0x80U : 0U));
    }
    return 0xFF;
}

void Machine::output(std::uint16_t port, std::uint8_t value) {
    const auto p = static_cast<std::uint8_t>(port);
    if (p >= 0xD0 && p <= 0xD3) {
        fdc_.write(static_cast<std::uint8_t>(p - 0xD0), value, drives_,
                   static_cast<std::uint8_t>(pio_b_output_ & 3U));
    } else if (p == 0xD5) {
        pio_b_output_ = static_cast<std::uint8_t>(value & 0x3FU);
    } else if (p == 0xD6) {
        // BIOS uses the standard PIO enable/disable interrupt words around
        // calls into the monitor ROM. Channel A receives keyboard ASTB.
        if ((value & 0x0FU) == 3U)
            pio_a_interrupt_enabled_ = (value & 0x80U) != 0;
    } else if (p == 0xD7) {
        // Z80 PIO mode 3 and interrupt-control words are followed by an I/O
        // direction or interrupt mask byte. Those bytes may be even, but they
        // are data and must not be mistaken for a new IM2 vector.
        if (pio_b_expect_direction_) {
            pio_b_expect_direction_ = false;
        } else if (pio_b_expect_interrupt_mask_) {
            pio_b_expect_interrupt_mask_ = false;
        } else if ((value & 1U) == 0) {
            pio_b_vector_ = value;
        } else if ((value & 0x0FU) == 0x0FU) {
            pio_b_expect_direction_ = (value & 0xC0U) == 0xC0U;
        } else if ((value & 0x0FU) == 7U) {
            pio_b_interrupt_enabled_ = (value & 0x80U) != 0;
            pio_b_expect_interrupt_mask_ = (value & 0x10U) != 0;
        } else if ((value & 0x0FU) == 3U) {
            pio_b_interrupt_enabled_ = (value & 0x80U) != 0;
        }
    } else if (p >= 0xF4 && p <= 0xF7) {
        cpu_ctc_.write(static_cast<std::uint8_t>(p - 0xF4), value);
    } else if (p == 0xFC) paging_ = value;
    else if ((p & 0xF0U) == 0xC0U) secondary_ = static_cast<std::uint8_t>(p & 0x0FU);
    // Remaining devices deliberately return benign values until their timing
    // models are introduced; decoding them here keeps the bus contract stable.
}

bool Machine::interrupt_pending() const {
    return cpu_ctc_.interrupt_pending() || (pio_b_interrupt_enabled_ && fdc_.drq()) ||
           (pio_a_interrupt_enabled_ && !keyboard_queue_.empty());
}

std::uint8_t Machine::interrupt_vector() {
    if (cpu_ctc_.interrupt_pending()) return cpu_ctc_.interrupt_acknowledge();
    if (pio_b_interrupt_enabled_ && fdc_.drq()) return pio_b_vector_;
    return static_cast<std::uint8_t>(pio_b_vector_ + 2U);
}

void Machine::key(std::uint8_t ascii) {
    constexpr std::size_t keyboard_buffer_size = 16;
    if (keyboard_queue_.size() < keyboard_buffer_size) keyboard_queue_.push_back(ascii);
}

} // namespace vz256
