#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace vz256 {

class FloppyImage {
public:
    static constexpr std::size_t sector_size = 512;
    static constexpr std::size_t sectors_per_track = 9;
    static constexpr std::size_t sides = 2;

    bool load(const std::filesystem::path& path);
    bool save() const;
    [[nodiscard]] bool mounted() const { return !bytes_.empty(); }
    [[nodiscard]] bool write_protected() const { return write_protected_; }
    [[nodiscard]] std::span<const std::uint8_t> sector(std::size_t track,
                                                       std::size_t side,
                                                       std::size_t one_based_sector) const;
    bool write_sector(std::size_t track, std::size_t side,
                      std::size_t one_based_sector, std::span<const std::uint8_t> data);

private:
    [[nodiscard]] std::size_t offset(std::size_t track, std::size_t side,
                                     std::size_t one_based_sector) const;
    std::filesystem::path path_;
    std::vector<std::uint8_t> bytes_;
    bool write_protected_{};
};

} // namespace vz256
