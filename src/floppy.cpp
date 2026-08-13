#include "vz256/floppy.hpp"

#include <fstream>
#include <iterator>

namespace vz256 {

bool FloppyImage::load(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(stream), {}};
    if (data.empty() || data.size() % (sector_size * sectors_per_track * sides) != 0) return false;
    path_ = path;
    bytes_ = std::move(data);
    std::ofstream probe(path, std::ios::binary | std::ios::app);
    write_protected_ = !probe.good();
    return true;
}

bool FloppyImage::save() const {
    if (!mounted() || write_protected_) return false;
    std::ofstream stream(path_, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes_.data()), static_cast<std::streamsize>(bytes_.size()));
    return stream.good();
}

std::size_t FloppyImage::offset(std::size_t track, std::size_t side,
                                std::size_t one_based_sector) const {
    if (side >= sides || one_based_sector == 0 || one_based_sector > sectors_per_track) return bytes_.size();
    return ((track * sides + side) * sectors_per_track + one_based_sector - 1) * sector_size;
}

std::span<const std::uint8_t> FloppyImage::sector(std::size_t track, std::size_t side,
                                                  std::size_t number) const {
    const auto begin = offset(track, side, number);
    if (begin + sector_size > bytes_.size()) return {};
    return {bytes_.data() + begin, sector_size};
}

bool FloppyImage::write_sector(std::size_t track, std::size_t side, std::size_t number,
                               std::span<const std::uint8_t> data) {
    const auto begin = offset(track, side, number);
    if (write_protected_ || data.size() != sector_size || begin + sector_size > bytes_.size()) return false;
    std::copy(data.begin(), data.end(), bytes_.begin() + static_cast<std::ptrdiff_t>(begin));
    return true;
}

} // namespace vz256
