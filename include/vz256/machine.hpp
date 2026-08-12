#pragma once
#include "vz256/wd2797.hpp"
#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <span>

namespace vz256 {
class Machine {
public:
  static constexpr int width = 640, height = 300;
  Machine();
  bool load_rom(const std::filesystem::path &path);
  bool mount_disk(const std::filesystem::path &path, bool writable = false);
  void reset();
  void run_cycles(std::uint32_t cycles);
  std::uint8_t read(std::uint16_t address, bool opcode = false) const;
  void write(std::uint16_t address, std::uint8_t value);
  std::uint8_t in(std::uint8_t port);
  void out(std::uint8_t port, std::uint8_t value);
  void key(std::uint8_t ascii);
  void render(std::span<std::uint32_t> pixels) const;
  [[nodiscard]] std::uint64_t cycles() const { return cycles_; }
  [[nodiscard]] std::uint8_t bank() const { return bank_; }
private:
  std::array<std::uint8_t, 256 * 1024> ram_{};
  std::array<std::uint8_t, 128 * 1024> video_{};
  std::array<std::uint8_t, 8 * 1024> board_ram_{};
  std::array<std::uint8_t, 8 * 1024> rom_{};
  std::array<std::uint8_t, 32> crtc_{};
  std::deque<std::uint8_t> keyboard_;
  Wd2797 fdc_;
  std::uint8_t bank_{}, crtc_index_{}, board_{}, pio_b_{0xff};
  std::uint64_t cycles_{};
};
}
