#include "vz256/floppy.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <system_error>

namespace vz256 {
namespace floppy_geometries {
namespace {
const std::array profiles{five_25_dsdd_80, five_25_dsdd_40, eight_sssd_77,
                          eight_dssd_77, eight_dsdd_77};
}

const FloppyGeometry* find(std::string_view name) {
    const auto found = std::find_if(profiles.begin(), profiles.end(),
                                    [name](const auto& profile) { return profile.name == name; });
    return found == profiles.end() ? nullptr : &*found;
}

const FloppyGeometry* detect(std::size_t image_size) {
    const FloppyGeometry* match = nullptr;
    for (const auto& profile : profiles) {
        if (profile.image_size() != image_size) continue;
        if (match != nullptr) return nullptr; // ambiguous sizes require an explicit profile
        match = &profile;
    }
    return match;
}
} // namespace floppy_geometries

bool FloppyImage::load(const std::filesystem::path& path, bool force_read_only) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(stream), {}};
    const auto* geometry = floppy_geometries::detect(data.size());
    if (geometry == nullptr) return false;
    return load(path, *geometry, force_read_only);
}

bool FloppyImage::load(const std::filesystem::path& path, FloppyGeometry geometry,
                       bool force_read_only) {
    if (!geometry.valid()) return false;
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(stream), {}};
    if (data.size() != geometry.image_size()) return false;
    path_ = path;
    bytes_ = std::move(data);
    geometry_ = geometry;
    std::ofstream probe;
    probe.open(path, std::ios::binary | std::ios::app);
    storage_writable_ = probe.good();
    write_protected_ = force_read_only || !storage_writable_;
    dirty_ = false;
    return true;
}

bool FloppyImage::save() const {
    if (!mounted() || write_protected_) return false;
    auto temporary = path_;
    temporary += ".vz256.tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        stream.write(reinterpret_cast<const char*>(bytes_.data()),
                     static_cast<std::streamsize>(bytes_.size()));
        stream.flush();
        if (!stream.good()) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return false;
        }
    }
    std::error_code error;
    std::filesystem::rename(temporary, path_, error);
    if (error) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    dirty_ = false;
    return true;
}

bool FloppyImage::set_write_protected(bool enabled) {
    if (!enabled && !storage_writable_) return false;
    write_protected_ = enabled;
    return true;
}

void FloppyImage::eject() {
    path_.clear();
    bytes_.clear();
    dirty_ = false;
    write_protected_ = false;
    storage_writable_ = false;
}

std::size_t FloppyImage::offset(std::size_t track, std::size_t side,
                                std::size_t one_based_sector) const {
    if (track >= geometry_.cylinders || side >= geometry_.sides || one_based_sector == 0 ||
        one_based_sector > geometry_.sectors_per_track) return bytes_.size();
    return ((track * geometry_.sides + side) * geometry_.sectors_per_track +
            one_based_sector - 1) * geometry_.sector_size;
}

std::span<const std::uint8_t> FloppyImage::sector(std::size_t track, std::size_t side,
                                                  std::size_t number) const {
    const auto begin = offset(track, side, number);
    if (begin + geometry_.sector_size > bytes_.size()) return {};
    return {bytes_.data() + begin, geometry_.sector_size};
}

bool FloppyImage::write_sector(std::size_t track, std::size_t side, std::size_t number,
                               std::span<const std::uint8_t> data) {
    const auto begin = offset(track, side, number);
    if (write_protected_ || data.size() != geometry_.sector_size ||
        begin + geometry_.sector_size > bytes_.size()) return false;
    std::copy(data.begin(), data.end(), bytes_.begin() + static_cast<std::ptrdiff_t>(begin));
    dirty_ = true;
    return true;
}

} // namespace vz256
