#include "vz256/cassette.hpp"

#include <array>
#include <fstream>
#include <iterator>
#include <string_view>
#include <utility>

namespace vz256 {
namespace {
std::uint16_t u16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | static_cast<std::uint16_t>(p[1]) << 8U);
}
std::uint32_t u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1]) << 8U |
           static_cast<std::uint32_t>(p[2]) << 16U | static_cast<std::uint32_t>(p[3]) << 24U;
}
}

bool Cassette::load_wav(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream), {}};
    if (bytes.size() < 12 || std::string_view(reinterpret_cast<const char*>(bytes.data()), 4) != "RIFF" ||
        std::string_view(reinterpret_cast<const char*>(bytes.data() + 8), 4) != "WAVE") return false;
    std::uint16_t channels = 0, bits = 0, format = 0;
    std::uint32_t rate = 0;
    const std::uint8_t* data = nullptr;
    std::size_t data_size = 0;
    for (std::size_t at = 12; at + 8 <= bytes.size();) {
        const auto size = static_cast<std::size_t>(u32(bytes.data() + at + 4));
        if (at + 8 + size > bytes.size()) return false;
        const std::string_view id(reinterpret_cast<const char*>(bytes.data() + at), 4);
        if (id == "fmt " && size >= 16) {
            format = u16(bytes.data() + at + 8); channels = u16(bytes.data() + at + 10);
            rate = u32(bytes.data() + at + 12); bits = u16(bytes.data() + at + 22);
        } else if (id == "data") { data = bytes.data() + at + 8; data_size = size; }
        at += 8 + size + (size & 1U);
    }
    if (format != 1 || (channels != 1 && channels != 2) || (bits != 8 && bits != 16) ||
        rate == 0 || data == nullptr) return false;
    const auto frame_size = static_cast<std::size_t>(channels) * (bits / 8U);
    if (frame_size == 0 || data_size % frame_size != 0) return false;
    std::vector<std::int16_t> decoded;
    decoded.reserve(data_size / frame_size * 2U);
    for (std::size_t at = 0; at < data_size; at += frame_size) {
        auto sample = [&](std::size_t channel) {
            const auto offset = at + channel * (bits / 8U);
            return bits == 8 ? static_cast<std::int16_t>((static_cast<int>(data[offset]) - 128) << 8)
                             : static_cast<std::int16_t>(u16(data + offset));
        };
        const auto left = sample(0); decoded.push_back(left);
        decoded.push_back(channels == 2 ? sample(1) : left);
    }
    samples_ = std::move(decoded); sample_rate_ = rate; rewind(); return true;
}

void Cassette::rewind() {
    position_ = 0; sample_phase_ = 0; left_level_ = false; right_level_ = false;
}

Cassette::Edges Cassette::tick(std::uint32_t cpu_cycles, std::uint32_t cpu_hz) {
    Edges edges;
    if (!motor_ || !loaded() || cpu_hz == 0) return edges;
    sample_phase_ += static_cast<std::uint64_t>(cpu_cycles) * sample_rate_;
    while (sample_phase_ >= cpu_hz && position_ < samples_.size() / 2U) {
        sample_phase_ -= cpu_hz;
        const bool left = samples_[position_ * 2U] >= 0;
        const bool right = samples_[position_ * 2U + 1U] >= 0;
        if (left != left_level_) ++edges.left;
        if (right != right_level_) ++edges.right;
        left_level_ = left; right_level_ = right; ++position_;
    }
    return edges;
}

} // namespace vz256
