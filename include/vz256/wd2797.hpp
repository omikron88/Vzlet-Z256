#pragma once

#include "vz256/floppy.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace vz256 {

class Wd2797 {
public:
    enum Status : std::uint8_t {
        busy = 0x01,
        data_request = 0x02,
        track_zero = 0x04,
        record_not_found = 0x10,
        write_protect = 0x40,
        not_ready = 0x80,
    };

    void reset();
    void tick(std::uint32_t cycles);
    [[nodiscard]] std::uint8_t read(std::uint8_t reg,
                                    std::array<FloppyImage, 4>& drives,
                                    std::uint8_t drive);
    void write(std::uint8_t reg, std::uint8_t value,
               std::array<FloppyImage, 4>& drives, std::uint8_t drive);

    [[nodiscard]] bool drq() const { return drq_; }
    [[nodiscard]] bool intrq() const { return intrq_; }

private:
    enum class Transfer { none, read, write, read_address };

    void command(std::uint8_t value, std::array<FloppyImage, 4>& drives,
                 std::uint8_t drive);
    bool begin_sector(std::array<FloppyImage, 4>& drives, std::uint8_t drive);
    void finish_sector(std::array<FloppyImage, 4>& drives, std::uint8_t drive);
    [[nodiscard]] std::uint8_t status(std::array<FloppyImage, 4>& drives,
                                      std::uint8_t drive);

    std::uint8_t status_{};
    std::uint8_t track_{};
    std::uint8_t sector_{1};
    std::uint8_t data_{};
    std::uint8_t command_{};
    bool drq_{};
    bool intrq_{};
    bool index_{};
    bool multiple_{};
    bool type_one_status_{true};
    std::uint8_t side_{};
    Transfer transfer_{Transfer::none};
    std::vector<std::uint8_t> buffer_;
    std::size_t position_{};
    std::uint32_t drq_delay_{};
};

} // namespace vz256
