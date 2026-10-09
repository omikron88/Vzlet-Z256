#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace vz256 {

class Cassette {
public:
    struct Edges { std::uint32_t left{}; std::uint32_t right{}; };
    bool load_wav(const std::filesystem::path& path);
    void rewind();
    void set_motor(bool running) { motor_ = running; }
    [[nodiscard]] bool motor() const { return motor_; }
    [[nodiscard]] bool loaded() const { return !samples_.empty(); }
    [[nodiscard]] bool finished() const { return loaded() && position_ >= samples_.size() / 2U; }
    [[nodiscard]] Edges tick(std::uint32_t cpu_cycles, std::uint32_t cpu_hz);

private:
    std::vector<std::int16_t> samples_; // interleaved stereo
    std::uint32_t sample_rate_{};
    std::uint64_t sample_phase_{};
    std::size_t position_{};
    bool left_level_{};
    bool right_level_{};
    bool motor_{};
};

} // namespace vz256
