#include "vz256/machine.hpp"
#include <algorithm>
#include <fstream>

namespace vz256 {
Machine::Machine() { reset(); }
bool Machine::load_rom(const std::filesystem::path &path) {
  std::ifstream f(path, std::ios::binary); if (!f) return false;
  rom_.fill(0xff); f.read(reinterpret_cast<char *>(rom_.data()), rom_.size()); return f.gcount() > 0;
}
bool Machine::mount_disk(const std::filesystem::path &p, bool w) { return fdc_.mount(p, w); }
void Machine::reset() { bank_ = board_ = crtc_index_ = 0; pio_b_ = 0xff; keyboard_.clear(); cycles_ = 0; }
std::uint8_t Machine::read(std::uint16_t a, bool opcode) const {
  if (!(bank_ & 0x80) && a < 0x2000) return rom_[a];
  if (!(bank_ & 0x80) && a >= 0xe000) return board_ram_[a - 0xe000];
  const auto page = opcode ? ((bank_ >> 4) & 3U) : (bank_ & 3U);
  return board_ == 1 ? video_[(page * 0x10000U + a) & (video_.size()-1)] : ram_[page * 0x10000U + a];
}
void Machine::write(std::uint16_t a, std::uint8_t v) {
  if (!(bank_ & 0x80) && a >= 0xe000) { board_ram_[a - 0xe000] = v; return; }
  const auto page = (bank_ >> 2) & 3U;
  if (board_ == 1) video_[(page * 0x10000U + a) & (video_.size()-1)] = v; else ram_[page * 0x10000U + a] = v;
}
std::uint8_t Machine::in(std::uint8_t p) {
  if (p >= 0xd0 && p <= 0xd3) return fdc_.read(p - 0xd0);
  if (p == 0xf0) { if (keyboard_.empty()) return 0; auto v=keyboard_.front(); keyboard_.pop_front(); return v; }
  if (p == 0xf1) return keyboard_.empty() ? 0xff : static_cast<std::uint8_t>(pio_b_ & ~1U);
  if (p == 0xc8) return crtc_[crtc_index_ & 31];
  return 0xff;
}
void Machine::out(std::uint8_t p, std::uint8_t v) {
  if (p >= 0xd0 && p <= 0xd3) { fdc_.write(p - 0xd0, v); return; }
  if (p == 0xd5) { fdc_.select(v); return; } if (p == 0xfc) { bank_ = v; return; }
  if (p >= 0xc0 && p <= 0xc2) { board_ = p - 0xc0; return; }
  if (p == 0xc8) crtc_[crtc_index_ & 31] = v; else if (p == 0xc9) crtc_index_ = v;
}
void Machine::key(std::uint8_t v) { if (keyboard_.size() < 16) keyboard_.push_back(v); }
void Machine::run_cycles(std::uint32_t n) {
  // CPU execution is supplied by redcode/Z80 in the front-end integration; peripherals use a common clock.
  cycles_ += n;
}
void Machine::render(std::span<std::uint32_t> px) const {
  if (px.size() < width * height) return;
  constexpr std::uint32_t grey[4]{0xff101410,0xff596159,0xffa5ada5,0xffe8eee8};
  for (int y=0;y<height;++y) for(int x=0;x<width;++x) {
    const auto bit = static_cast<std::size_t>(y)*width+x, byte=bit/4;
    const auto shift=6-2*(bit&3); px[bit]=grey[(video_[byte]>>shift)&3];
  }
}
}
