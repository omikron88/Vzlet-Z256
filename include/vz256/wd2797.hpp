#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace vz256 {
class Wd2797 {
public:
  bool mount(const std::filesystem::path &path, bool writable = false);
  void select(std::uint8_t control);
  std::uint8_t read(std::uint8_t reg);
  void write(std::uint8_t reg, std::uint8_t value);
  bool flush();
  [[nodiscard]] bool mounted() const { return !image_.empty(); }
private:
  void command(std::uint8_t value);
  std::size_t offset() const;
  std::vector<std::uint8_t> image_, transfer_;
  std::filesystem::path path_;
  std::size_t position_{};
  std::uint8_t status_{0x80}, track_{}, sector_{1}, data_{}, control_{};
  bool writable_{}, writing_{};
};
}
