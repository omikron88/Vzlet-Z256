#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <vector>

namespace vz256 {

class Sio {
public:
    void reset();
    [[nodiscard]] std::uint8_t read_data(std::uint8_t channel);
    [[nodiscard]] std::uint8_t read_control(std::uint8_t channel);
    void write_data(std::uint8_t channel, std::uint8_t value);
    void write_control(std::uint8_t channel, std::uint8_t value);
    void receive(std::uint8_t channel, std::uint8_t value);
    [[nodiscard]] std::vector<std::uint8_t> take_transmitted(std::uint8_t channel);
    [[nodiscard]] bool rts(std::uint8_t channel) const;
    [[nodiscard]] bool dtr(std::uint8_t channel) const;
    [[nodiscard]] bool interrupt_pending() const;
    [[nodiscard]] std::uint8_t interrupt_acknowledge();
    [[nodiscard]] bool interrupt_reti();

private:
    struct Channel {
        std::array<std::uint8_t, 8> registers{};
        std::deque<std::uint8_t> received;
        std::vector<std::uint8_t> transmitted;
        std::uint8_t selected_register{};
        bool expect_register_data{};
        bool receive_interrupt{};
        bool interrupt_in_service{};
    };

    [[nodiscard]] static bool receiver_enabled(const Channel& channel);
    [[nodiscard]] static bool transmitter_enabled(const Channel& channel);
    [[nodiscard]] static bool receive_interrupt_enabled(const Channel& channel);
    std::array<Channel, 2> channels_{};
    std::uint8_t vector_{};
};

} // namespace vz256
