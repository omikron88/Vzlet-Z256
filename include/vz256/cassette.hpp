#pragma once

#include <cstdint>
#include <array>
#include <deque>
#include <filesystem>
#include <vector>

namespace vz256 {

class Cassette {
public:
    struct Edges { std::uint32_t left{}; std::uint32_t right{}; };
    bool load_wav(const std::filesystem::path& path);
    bool create_wav(const std::filesystem::path& path, std::uint32_t sample_rate = 44'100);
    bool save_wav();
    void record_byte(std::uint8_t channel, std::uint8_t value,
                     std::uint32_t baud = 9'600, bool invert = false);
    void set_decode_baud(std::uint8_t channel, std::uint32_t baud, bool invert = false);
    [[nodiscard]] std::vector<std::uint8_t> take_decoded(std::uint8_t channel);
    void rewind();
    void set_motor(bool running) { motor_ = running; }
    [[nodiscard]] bool motor() const { return motor_; }
    [[nodiscard]] bool loaded() const { return mounted_; }
    [[nodiscard]] bool recording() const { return recording_; }
    [[nodiscard]] bool finished() const { return loaded() && position_ >= samples_.size() / 2U; }
    [[nodiscard]] Edges tick(std::uint32_t cpu_cycles, std::uint32_t cpu_hz);

private:
    struct Decoder {
        std::deque<std::uint8_t> bytes;
        std::uint64_t phase{};
        std::uint64_t half_period_fp{};
        std::int64_t sum{};
        std::uint32_t samples{};
        std::uint32_t edge_samples{};
        std::uint32_t baud{};
        std::uint8_t data{};
        int bit{-1};
        bool first_half{};
        bool have_first{};
        bool level{};
        bool level_initialized{};
        bool input_level{};
        bool input_initialized{};
        bool invert{};
    };
    void decode_sample(std::uint8_t channel, std::int16_t sample);
    static void finish_decode_half(Decoder& decoder);
    static void decode_half(Decoder& decoder, bool level);
    std::vector<std::int16_t> samples_; // interleaved stereo
    std::uint32_t sample_rate_{};
    std::uint64_t sample_phase_{};
    std::uint64_t record_phase_{};
    std::size_t position_{};
    std::filesystem::path path_;
    bool left_level_{};
    bool right_level_{};
    bool levels_initialized_{};
    bool motor_{};
    bool mounted_{};
    bool recording_{};
    std::array<Decoder, 2> decoders_{};
};

} // namespace vz256
