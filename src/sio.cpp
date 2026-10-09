#include "vz256/sio.hpp"

#include <utility>

namespace vz256 {

void Sio::reset() {
    channels_ = {};
    vector_ = 0;
}

bool Sio::receiver_enabled(const Channel& channel) {
    return (channel.registers[3] & 0x01U) != 0;
}

bool Sio::transmitter_enabled(const Channel& channel) {
    return (channel.registers[5] & 0x08U) != 0;
}

bool Sio::receive_interrupt_enabled(const Channel& channel) {
    return (channel.registers[1] & 0x18U) != 0;
}

std::uint8_t Sio::read_data(std::uint8_t channel_number) {
    auto& channel = channels_.at(channel_number & 1U);
    if (channel.received.empty()) return 0;
    const auto value = channel.received.front();
    channel.received.pop_front();
    channel.receive_interrupt = receive_interrupt_enabled(channel) && !channel.received.empty();
    return value;
}

std::uint8_t Sio::read_control(std::uint8_t channel_number) {
    auto& channel = channels_.at(channel_number & 1U);
    if (channel.expect_register_data) {
        channel.expect_register_data = false;
        if (channel.selected_register == 2U && (channel_number & 1U) == 1U)
            return vector_;
        if (channel.selected_register != 0U) return 0;
    }
    // RR0: receive character available, transmitter buffer empty, DCD and CTS.
    return static_cast<std::uint8_t>((channel.received.empty() ? 0U : 0x01U) |
                                     0x04U | 0x08U | 0x20U);
}

void Sio::write_data(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 1U);
    if (transmitter_enabled(channel)) channel.transmitted.push_back(value);
}

void Sio::write_control(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 1U);
    if (channel.expect_register_data) {
        channel.expect_register_data = false;
        channel.registers[channel.selected_register] = value;
        if ((channel_number & 1U) == 1U && channel.selected_register == 2U)
            vector_ = value;
        if (channel.selected_register == 1U)
            channel.receive_interrupt = receive_interrupt_enabled(channel) &&
                                        !channel.received.empty();
        return;
    }

    channel.selected_register = static_cast<std::uint8_t>(value & 7U);
    if (channel.selected_register != 0U) {
        channel.expect_register_data = true;
        return;
    }

    switch ((value >> 3U) & 7U) {
    case 2: // reset external/status interrupt
        break;
    case 3: // channel reset
        channel = {};
        break;
    case 5: // reset transmitter interrupt
        break;
    case 6: // error reset
        break;
    case 7: // return from interrupt
        channel.receive_interrupt = receive_interrupt_enabled(channel) &&
                                    !channel.received.empty();
        break;
    default: break;
    }
}

void Sio::receive(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 1U);
    if (!receiver_enabled(channel)) return;
    channel.received.push_back(value);
    if (receive_interrupt_enabled(channel)) channel.receive_interrupt = true;
}

std::vector<std::uint8_t> Sio::take_transmitted(std::uint8_t channel_number) {
    auto& output = channels_.at(channel_number & 1U).transmitted;
    auto result = std::move(output);
    output.clear();
    return result;
}

bool Sio::interrupt_pending() const {
    return channels_[0].receive_interrupt || channels_[1].receive_interrupt;
}

std::uint8_t Sio::interrupt_acknowledge() {
    // Channel A has the higher daisy-chain priority. With status-affects-vector
    // disabled, both channels supply the vector programmed in channel B WR2.
    if (channels_[0].receive_interrupt) channels_[0].receive_interrupt = false;
    else if (channels_[1].receive_interrupt) channels_[1].receive_interrupt = false;
    return vector_;
}

} // namespace vz256
