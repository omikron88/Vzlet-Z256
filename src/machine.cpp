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

void Machine::reset() {
    paging_ = 0;
    secondary_ = 0;
    keyboard_ready_ = false;
    video_.reset();
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
    if (p == 0xD4) {
        keyboard_ready_ = false;
        return keyboard_data_;
    }
    if (p == 0xD6) return keyboard_ready_ ? 0x80 : 0x00;
    return 0xFF;
}

void Machine::output(std::uint16_t port, std::uint8_t value) {
    const auto p = static_cast<std::uint8_t>(port);
    if (p == 0xFC) paging_ = value;
    else if ((p & 0xF0U) == 0xC0U) secondary_ = static_cast<std::uint8_t>(p & 0x0FU);
    // Remaining devices deliberately return benign values until their timing
    // models are introduced; decoding them here keeps the bus contract stable.
}

void Machine::key(std::uint8_t ascii) {
    keyboard_data_ = ascii;
    keyboard_ready_ = true;
}

} // namespace vz256
