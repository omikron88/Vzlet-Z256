#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace vz256 {

enum class FloppyEncoding { fm, mfm };

struct FloppyGeometry {
    std::string name;
    std::size_t cylinders;
    std::size_t sides;
    std::size_t sectors_per_track;
    std::size_t sector_size;
    FloppyEncoding encoding;
    bool double_step{};

    [[nodiscard]] constexpr std::size_t image_size() const {
        return cylinders * sides * sectors_per_track * sector_size;
    }
    [[nodiscard]] constexpr bool valid() const {
        return cylinders != 0 && sides != 0 && sides <= 2 && sectors_per_track != 0 &&
               sector_size >= 128 && sector_size <= 1024 &&
               (sector_size & (sector_size - 1)) == 0;
    }
};

namespace floppy_geometries {
inline const FloppyGeometry five_25_dsdd_80{"5.25-dsdd-80", 80, 2, 9, 512,
                                            FloppyEncoding::mfm};
inline const FloppyGeometry five_25_dsdd_40{"5.25-dsdd-40", 40, 2, 9, 512,
                                            FloppyEncoding::mfm, true};
inline const FloppyGeometry eight_sssd_77{"8-sssd-77", 77, 1, 26, 128,
                                          FloppyEncoding::fm};
inline const FloppyGeometry eight_dssd_77{"8-dssd-77", 77, 2, 26, 128,
                                          FloppyEncoding::fm};
inline const FloppyGeometry eight_dsdd_77{"8-dsdd-77", 77, 2, 26, 256,
                                          FloppyEncoding::mfm};

[[nodiscard]] const FloppyGeometry* find(std::string_view name);
[[nodiscard]] const FloppyGeometry* detect(std::size_t image_size);
} // namespace floppy_geometries

class FloppyImage {
public:
    // Legacy aliases for the default image; new code should query geometry().
    static constexpr std::size_t sector_size = 512;
    static constexpr std::size_t sectors_per_track = 9;
    static constexpr std::size_t sides = 2;

    bool load(const std::filesystem::path& path, bool force_read_only = false);
    bool load(const std::filesystem::path& path, FloppyGeometry geometry,
              bool force_read_only = false);
    bool save() const;
    void eject();
    [[nodiscard]] bool mounted() const { return !bytes_.empty(); }
    [[nodiscard]] bool write_protected() const { return write_protected_; }
    [[nodiscard]] bool dirty() const { return dirty_; }
    [[nodiscard]] const FloppyGeometry& geometry() const { return geometry_; }
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
    FloppyGeometry geometry_{floppy_geometries::five_25_dsdd_80};
    bool write_protected_{};
    mutable bool dirty_{};
};

} // namespace vz256
