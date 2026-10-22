#pragma once

#include <array>
#include <cstdint>

namespace vz256 {

class Pio {
public:
    void reset();
    [[nodiscard]] std::uint8_t read(std::uint8_t channel);
    void write(std::uint8_t channel, std::uint8_t value);
    void control(std::uint8_t channel, std::uint8_t value);
    void set_input(std::uint8_t channel, std::uint8_t value);
    void strobe(std::uint8_t channel);
    [[nodiscard]] std::uint8_t output(std::uint8_t channel) const;
    [[nodiscard]] bool interrupt_pending() const;
    [[nodiscard]] std::uint8_t interrupt_acknowledge();
    [[nodiscard]] bool interrupt_reti();

private:
    struct Channel {
        std::uint8_t input{0xff};
        std::uint8_t output{};
        std::uint8_t vector{};
        std::uint8_t direction{0xff};
        std::uint8_t interrupt_mask{0xff};
        std::uint8_t mode{1};
        bool interrupt_enabled{};
        bool interrupt_and{};
        bool interrupt_high{};
        bool expect_direction{};
        bool expect_interrupt_mask{};
        bool interrupt_pending{};
        bool interrupt_in_service{};
    };

    void update_level_interrupt(Channel& channel);
    std::array<Channel, 2> channels_{};
};

} // namespace vz256
