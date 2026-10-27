#include "vz256/floppy.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <system_error>

namespace vz256 {
namespace {
std::uint8_t sector_size_code(std::size_t size) {
    std::uint8_t code = 0;
    for (std::size_t value = 128; value < size; value *= 2) ++code;
    return code;
}
}
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
    if (data.size() >= 4 && std::equal(data.begin(), data.begin() + 4, "IMD "))
        return load_imd(path, data, force_read_only);
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
    if (data.size() >= 4 && std::equal(data.begin(), data.begin() + 4, "IMD "))
        return load_imd(path, data, force_read_only);
    if (data.size() != geometry.image_size()) return false;
    path_ = path;
    bytes_ = std::move(data);
    geometry_ = geometry;
    initialize_sector_metadata();
    imd_format_ = false;
    std::ofstream probe;
    probe.open(path, std::ios::binary | std::ios::app);
    storage_writable_ = probe.good();
    write_protected_ = force_read_only || !storage_writable_;
    dirty_ = false;
    return true;
}

bool FloppyImage::load(std::span<const std::uint8_t> bytes, FloppyGeometry geometry) {
    if (!geometry.valid() || bytes.size() != geometry.image_size()) return false;
    path_ = "<embedded boot.img>";
    bytes_.assign(bytes.begin(), bytes.end());
    geometry_ = std::move(geometry);
    initialize_sector_metadata();
    imd_format_ = false;
    storage_writable_ = false;
    write_protected_ = true;
    dirty_ = false;
    return true;
}

bool FloppyImage::create(const std::filesystem::path& path, FloppyGeometry geometry,
                         std::uint8_t fill) {
    if (!geometry.valid() || path.empty()) return false;
    FloppyImage image;
    image.path_ = path;
    image.bytes_.assign(geometry.image_size(), fill);
    image.geometry_ = std::move(geometry);
    image.initialize_sector_metadata();
    image.imd_format_ = false;
    image.storage_writable_ = true;
    image.write_protected_ = false;
    image.dirty_ = true;
    if (!image.save()) return false;
    *this = std::move(image);
    return true;
}

bool FloppyImage::save() const {
    if (!mounted() || write_protected_) return false;
    auto temporary = path_;
    temporary += ".vz256.tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (imd_format_) {
            stream << "IMD 1.20: Vzlet Z-256\r\n";
            stream.put(0x1a);
            const auto mode = geometry_.encoding == FloppyEncoding::mfm
                ? (geometry_.rpm == 360 ? 3 : 5)
                : (geometry_.rpm == 360 ? 0 : 2);
            for (std::size_t track = 0; track < geometry_.cylinders; ++track) {
                for (std::size_t side = 0; side < geometry_.sides; ++side) {
                    stream.put(static_cast<char>(mode));
                    stream.put(static_cast<char>(track));
                    stream.put(static_cast<char>(side));
                    stream.put(static_cast<char>(geometry_.sectors_per_track));
                    const auto first_size = sector_metadata(track, side, 1).size;
                    const bool uniform_size = [&] {
                        for (std::size_t slot = 2; slot <= geometry_.sectors_per_track; ++slot)
                            if (sector_metadata(track, side, slot).size != first_size) return false;
                        return true;
                    }();
                    const bool standard_size = first_size >= 128 && first_size <= 8192 &&
                        (first_size & (first_size - 1U)) == 0;
                    stream.put(static_cast<char>(uniform_size && standard_size
                        ? sector_size_code(first_size) : 0xffU));
                    for (std::size_t slot = 1; slot <= geometry_.sectors_per_track; ++slot)
                        stream.put(static_cast<char>(sector_metadata(track, side, slot).id));
                    if (!uniform_size || !standard_size) {
                        for (std::size_t slot = 1; slot <= geometry_.sectors_per_track; ++slot) {
                            const auto size = sector_metadata(track, side, slot).size;
                            stream.put(static_cast<char>(size));
                            stream.put(static_cast<char>(size >> 8U));
                        }
                    }
                    for (std::size_t slot = 1; slot <= geometry_.sectors_per_track; ++slot) {
                        const auto metadata = sector_metadata(track, side, slot);
                        const auto data = sector(track, side, slot);
                        const bool compressed = !data.empty() &&
                            std::all_of(data.begin(), data.end(),
                                        [first = data.front()](auto byte) { return byte == first; });
                        auto type = metadata.unavailable ? 0U
                            : metadata.deleted_data ? 3U : 1U;
                        if (!metadata.unavailable && metadata.crc_error) type += 4U;
                        if (!metadata.unavailable && compressed) ++type;
                        stream.put(static_cast<char>(type));
                        if (!metadata.unavailable) {
                            if (compressed) stream.put(static_cast<char>(data.front()));
                            else stream.write(reinterpret_cast<const char*>(data.data()),
                                              static_cast<std::streamsize>(data.size()));
                        }
                    }
                }
            }
        } else {
            stream.write(reinterpret_cast<const char*>(bytes_.data()),
                         static_cast<std::streamsize>(bytes_.size()));
        }
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
        // Windows does not replace an existing destination with rename(). Keep
        // the original recoverable until the new, completely written file is
        // in place.
        auto backup = path_;
        backup += ".vz256.bak";
        std::error_code ignored;
        std::filesystem::remove(backup, ignored);
        error.clear();
        std::filesystem::rename(path_, backup, error);
        if (!error) {
            std::filesystem::rename(temporary, path_, error);
            if (error) {
                std::error_code restore_error;
                std::filesystem::rename(backup, path_, restore_error);
            } else {
                std::filesystem::remove(backup, ignored);
            }
        }
        if (error) {
            std::filesystem::remove(temporary, ignored);
            return false;
        }
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
    sector_metadata_.clear();
    sector_offsets_.clear();
    dirty_ = false;
    write_protected_ = false;
    storage_writable_ = false;
    imd_format_ = false;
}

std::size_t FloppyImage::offset(std::size_t track, std::size_t side,
                                std::size_t one_based_sector) const {
    if (track >= geometry_.cylinders || side >= geometry_.sides || one_based_sector == 0 ||
        one_based_sector > geometry_.sectors_per_track) return bytes_.size();
    return sector_offsets_[((track * geometry_.sides + side) *
                            geometry_.sectors_per_track) + one_based_sector - 1];
}

std::span<const std::uint8_t> FloppyImage::sector(std::size_t track, std::size_t side,
                                                  std::size_t number) const {
    const auto begin = offset(track, side, number);
    const auto metadata = sector_metadata(track, side, number);
    if (begin + metadata.size > bytes_.size()) return {};
    return {bytes_.data() + begin, metadata.size};
}

std::span<const std::uint8_t> FloppyImage::sector_by_id(std::size_t track,
                                                        std::size_t side,
                                                        std::uint8_t id) const {
    if (track >= geometry_.cylinders || side >= geometry_.sides) return {};
    const auto base = (track * geometry_.sides + side) * geometry_.sectors_per_track;
    for (std::size_t slot = 0; slot < geometry_.sectors_per_track; ++slot) {
        if (sector_metadata_[base + slot].id == id &&
            !sector_metadata_[base + slot].unavailable)
            return sector(track, side, slot + 1);
    }
    return {};
}

std::size_t FloppyImage::sector_slot_by_id(std::size_t track, std::size_t side,
                                            std::uint8_t id) const {
    if (track >= geometry_.cylinders || side >= geometry_.sides)
        return geometry_.sectors_per_track;
    const auto base = (track * geometry_.sides + side) * geometry_.sectors_per_track;
    for (std::size_t slot = 0; slot < geometry_.sectors_per_track; ++slot)
        if (sector_metadata_[base + slot].id == id) return slot;
    return geometry_.sectors_per_track;
}

FloppyImage::SectorMetadata FloppyImage::sector_metadata(
    std::size_t track, std::size_t side, std::size_t one_based_sector) const {
    if (track >= geometry_.cylinders || side >= geometry_.sides ||
        one_based_sector == 0 || one_based_sector > geometry_.sectors_per_track)
        return {};
    return sector_metadata_[((track * geometry_.sides + side) *
                             geometry_.sectors_per_track) + one_based_sector - 1];
}

void FloppyImage::initialize_sector_metadata() {
    sector_metadata_.resize(geometry_.cylinders * geometry_.sides *
                            geometry_.sectors_per_track);
    sector_offsets_.resize(sector_metadata_.size());
    for (std::size_t track = 0; track < geometry_.cylinders; ++track)
        for (std::size_t side = 0; side < geometry_.sides; ++side)
            for (std::size_t slot = 0; slot < geometry_.sectors_per_track; ++slot)
                {
                    const auto index = ((track * geometry_.sides + side) *
                                        geometry_.sectors_per_track) + slot;
                    sector_metadata_[index].id = static_cast<std::uint8_t>(slot + 1);
                    sector_metadata_[index].size = geometry_.sector_size;
                    sector_offsets_[index] = index * geometry_.sector_size;
                }
}

bool FloppyImage::load_imd(const std::filesystem::path& path,
                           std::span<const std::uint8_t> file, bool force_read_only) {
    const auto header_end = std::find(file.begin(), file.end(), 0x1aU);
    if (header_end == file.end()) return false;
    std::size_t cursor = static_cast<std::size_t>(header_end - file.begin()) + 1;
    struct Track {
        std::uint8_t cylinder;
        std::uint8_t side;
        std::uint8_t mode;
        std::uint8_t size_code;
        std::vector<std::uint8_t> ids;
        std::vector<std::size_t> sizes;
        std::vector<std::vector<std::uint8_t>> sectors;
        std::vector<SectorMetadata> metadata;
    };
    std::vector<Track> tracks;
    std::size_t cylinders = 0, sides = 0, sectors_per_track = 0, sector_size = 0;
    while (cursor < file.size()) {
        if (file.size() - cursor < 5) return false;
        Track track{file[cursor + 1], static_cast<std::uint8_t>(file[cursor + 2] & 1U),
                    file[cursor], file[cursor + 4], {}, {}, {}, {}};
        const auto head_flags = file[cursor + 2];
        const auto count = file[cursor + 3];
        cursor += 5;
        if (count == 0 || (track.size_code > 6 && track.size_code != 0xffU) ||
            file.size() - cursor < count)
            return false;
        const auto common_size = track.size_code == 0xffU ? 0U
                                                          : std::size_t{128} << track.size_code;
        track.ids.assign(file.begin() + static_cast<std::ptrdiff_t>(cursor),
                         file.begin() + static_cast<std::ptrdiff_t>(cursor + count));
        cursor += count;
        if ((head_flags & 0x80U) != 0) { // optional per-sector cylinder map
            if (file.size() - cursor < count) return false;
            cursor += count;
        }
        if ((head_flags & 0x40U) != 0) { // optional per-sector head map
            if (file.size() - cursor < count) return false;
            cursor += count;
        }
        if (track.size_code == 0xffU) {
            if (file.size() - cursor < count * 2U) return false;
            for (std::size_t slot = 0; slot < count; ++slot) {
                const auto entry = static_cast<std::size_t>(file[cursor + slot * 2U]) |
                    (static_cast<std::size_t>(file[cursor + slot * 2U + 1]) << 8U);
                if (entry < 128 || entry > 8192) return false;
                track.sizes.push_back(entry);
            }
            cursor += count * 2U;
        } else track.sizes.assign(count, common_size);
        track.sectors.resize(count);
        for (std::size_t slot = 0; slot < count; ++slot)
            track.sectors[slot].assign(track.sizes[slot], 0xe5U);
        track.metadata.resize(count);
        for (std::size_t slot = 0; slot < count; ++slot) {
            if (cursor >= file.size()) return false;
            const auto type = file[cursor++];
            if (type > 8) return false;
            track.metadata[slot] = {track.ids[slot], type == 3 || type == 4 ||
                                                        type == 7 || type == 8,
                                    type == 5 || type == 6 || type == 7 || type == 8,
                                    type == 0, track.sizes[slot]};
            if (type == 0) continue;
            if ((type & 1U) != 0) {
                if (file.size() - cursor < track.sizes[slot]) return false;
                std::copy_n(file.begin() + static_cast<std::ptrdiff_t>(cursor),
                            track.sizes[slot],
                            track.sectors[slot].begin());
                cursor += track.sizes[slot];
            } else {
                if (cursor >= file.size()) return false;
                std::fill(track.sectors[slot].begin(), track.sectors[slot].end(),
                          file[cursor++]);
            }
        }
        if (!tracks.empty() && count != sectors_per_track) return false;
        sectors_per_track = count;
        sector_size = std::max(sector_size, *std::max_element(track.sizes.begin(),
                                                              track.sizes.end()));
        cylinders = std::max(cylinders, static_cast<std::size_t>(track.cylinder) + 1);
        sides = std::max(sides, static_cast<std::size_t>(track.side) + 1);
        tracks.push_back(std::move(track));
    }
    if (tracks.empty()) return false;
    const auto mode = tracks.front().mode;
    if (mode > 5) return false;
    geometry_ = {"imd", cylinders, sides, sectors_per_track, sector_size,
                 mode >= 3 ? FloppyEncoding::mfm : FloppyEncoding::fm, false,
                 static_cast<std::uint16_t>(mode == 0 || mode == 3 ? 360 : 300)};
    bytes_.clear();
    sector_metadata_.assign(cylinders * sides * sectors_per_track, {});
    sector_offsets_.assign(sector_metadata_.size(), 0);
    std::vector<bool> populated(cylinders * sides, false);
    for (const auto& track : tracks) {
        const auto track_index = static_cast<std::size_t>(track.cylinder) * sides + track.side;
        if (populated[track_index]) return false;
        populated[track_index] = true;
        for (std::size_t slot = 0; slot < sectors_per_track; ++slot) {
            const auto index = ((static_cast<std::size_t>(track.cylinder) * sides +
                                 track.side) * sectors_per_track) + slot;
            sector_offsets_[index] = bytes_.size();
            bytes_.insert(bytes_.end(), track.sectors[slot].begin(), track.sectors[slot].end());
            sector_metadata_[index] = track.metadata[slot];
        }
    }
    if (!std::all_of(populated.begin(), populated.end(), [](bool value) { return value; }))
        return false;
    path_ = path;
    std::ofstream probe(path, std::ios::binary | std::ios::app);
    storage_writable_ = probe.good();
    write_protected_ = force_read_only || !storage_writable_;
    imd_format_ = true;
    dirty_ = false;
    return true;
}

bool FloppyImage::write_sector(std::size_t track, std::size_t side, std::size_t number,
                               std::span<const std::uint8_t> data) {
    const auto begin = offset(track, side, number);
    const auto metadata = sector_metadata(track, side, number);
    if (write_protected_ || data.size() != metadata.size ||
        begin + metadata.size > bytes_.size()) return false;
    std::copy(data.begin(), data.end(), bytes_.begin() + static_cast<std::ptrdiff_t>(begin));
    dirty_ = true;
    return true;
}

} // namespace vz256
