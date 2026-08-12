#include "vz256/wd2797.hpp"
#include <fstream>

namespace vz256 {
bool Wd2797::mount(const std::filesystem::path &path, bool writable) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  image_ = {std::istreambuf_iterator<char>(f), {}};
  path_ = path; writable_ = writable; status_ = 0; return true;
}
void Wd2797::select(std::uint8_t control) { control_ = control; }
std::size_t Wd2797::offset() const {
  // Supplied image is 80 tracks, two sides, 18 256-byte sectors.
  const auto side = (control_ >> 6) & 1U;
  return ((static_cast<std::size_t>(track_) * 2 + side) * 18 + (sector_ ? sector_ - 1 : 0)) * 256;
}
void Wd2797::command(std::uint8_t value) {
  status_ = 0; position_ = 0; transfer_.clear(); writing_ = false;
  if ((value & 0xf0) == 0x00) { track_ = 0; return; }
  if ((value & 0xf0) == 0x10) { track_ = data_; return; }
  const auto off = offset();
  if ((value & 0xe0) == 0x80) {
    if (off + 256 > image_.size()) { status_ = 0x10; return; }
    transfer_.assign(image_.begin() + off, image_.begin() + off + 256); status_ = 0x03;
  } else if ((value & 0xe0) == 0xa0) {
    if (!writable_ || off + 256 > image_.size()) { status_ = 0x40; return; }
    transfer_.assign(256, 0); writing_ = true; status_ = 0x03;
  }
}
std::uint8_t Wd2797::read(std::uint8_t reg) {
  if (reg == 0) return status_;
  if (reg == 1) return track_;
  if (reg == 2) return sector_;
  if (position_ >= transfer_.size()) return data_;
  data_ = transfer_[position_++];
  if (position_ == transfer_.size()) status_ = 0;
  return data_;
}
void Wd2797::write(std::uint8_t reg, std::uint8_t value) {
  if (reg == 0) return command(value);
  if (reg == 1) { track_ = value; return; }
  if (reg == 2) { sector_ = value; return; } data_ = value;
  if (writing_ && position_ < transfer_.size()) {
    transfer_[position_++] = value;
    if (position_ == transfer_.size()) { std::copy(transfer_.begin(), transfer_.end(), image_.begin() + offset()); status_ = 0; writing_ = false; }
  }
}
bool Wd2797::flush() {
  if (!writable_ || path_.empty()) return false;
  std::ofstream f(path_, std::ios::binary | std::ios::trunc); f.write(reinterpret_cast<const char *>(image_.data()), image_.size()); return !!f;
}
}
