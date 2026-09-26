#pragma once

#include <array>
#include <cstdint>

namespace vz256 {

class Ctc {
public:
    void reset();
    void write(std::uint8_t channel, std::uint8_t value);
    [[nodiscard]] std::uint8_t read(std::uint8_t channel) const;
    void tick(std::uint32_t cycles);
    void trigger(std::uint8_t channel, std::uint32_t pulses = 1);
    [[nodiscard]] bool interrupt_pending() const;
    [[nodiscard]] std::uint8_t interrupt_acknowledge();

private:
    struct Channel {
        std::uint16_t constant{};
        std::uint16_t counter{};
        std::uint16_t prescaler{16};
        std::uint32_t prescaler_cycles{};
        bool counter_mode{};
        bool external_trigger{};
        bool interrupt_enabled{};
        bool expect_constant{};
        bool running{};
        bool interrupt_pending{};
    };

    std::uint32_t advance(std::uint8_t channel, std::uint32_t ticks);
    std::array<Channel, 4> channels_{};
    std::uint8_t vector_{};
};

} // namespace vz256
