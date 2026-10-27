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

bool Sio::transmit_interrupt_enabled(const Channel& channel) {
    return (channel.registers[1] & 0x02U) != 0;
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
            return pending_vector();
        if (channel.selected_register != 0U) return 0;
    }
    // RR0: receive character available, transmitter buffer empty, DCD and CTS.
    return static_cast<std::uint8_t>((channel.received.empty() ? 0U : 0x01U) |
                                     0x04U | 0x08U | 0x20U);
}

bool Sio::write_data(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 1U);
    if (!transmitter_enabled(channel)) return false;
    channel.transmitted.push_back(value);
    // Bytes leave the simplified, unbuffered transmitter immediately.  The
    // resulting empty-buffer condition is the Z80 SIO transmit interrupt.
    if (transmit_interrupt_enabled(channel)) channel.transmit_interrupt = true;
    return true;
}

void Sio::write_control(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 1U);
    if (channel.expect_register_data) {
        channel.expect_register_data = false;
        const auto selected_register = channel.selected_register;
        channel.registers[selected_register] = value;
        if ((channel_number & 1U) == 1U && channel.selected_register == 2U)
            vector_ = value;
        if (selected_register == 1U) {
            channel.receive_interrupt = receive_interrupt_enabled(channel) &&
                                        !channel.received.empty();
            if (!transmit_interrupt_enabled(channel))
                channel.transmit_interrupt = false;
        }
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
    case 5: // reset transmitter interrupt pending
        channel.transmit_interrupt = false;
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

bool Sio::rts(std::uint8_t channel_number) const {
    return (channels_.at(channel_number & 1U).registers[5] & 0x02U) != 0;
}

bool Sio::dtr(std::uint8_t channel_number) const {
    return (channels_.at(channel_number & 1U).registers[5] & 0x80U) != 0;
}

std::uint32_t Sio::clock_divisor(std::uint8_t channel_number) const {
    static constexpr std::array<std::uint32_t, 4> divisors{1, 16, 32, 64};
    return divisors[(channels_.at(channel_number & 1U).registers[4] >> 6U) & 3U];
}

bool Sio::interrupt_pending() const {
    for (const auto& channel : channels_) {
        if (channel.receive_interrupt || channel.transmit_interrupt) return true;
        if (channel.interrupt_in_service) return false;
    }
    return false;
}

std::uint8_t Sio::modified_vector(std::uint8_t status) const {
    // Channel B WR1 D2 replaces vector bits 1..3 with the interrupt source:
    // B Tx=000, B Rx=010, A Tx=100 and A Rx=110. Bit zero is preserved even
    // though a Z80 running in IM2 ignores it when forming the table address.
    if ((channels_[1].registers[1] & 0x04U) == 0) return vector_;
    return static_cast<std::uint8_t>((vector_ & 0xf1U) | ((status & 7U) << 1U));
}

std::uint8_t Sio::pending_vector() const {
    if (channels_[0].receive_interrupt) return modified_vector(6);
    if (channels_[0].transmit_interrupt) return modified_vector(4);
    if (channels_[1].receive_interrupt) return modified_vector(2);
    if (channels_[1].transmit_interrupt) return modified_vector(0);
    return vector_;
}

std::uint8_t Sio::interrupt_acknowledge() {
    // Channel A has the higher daisy-chain priority and receive precedes
    // transmit within each channel.
    const auto acknowledged_vector = pending_vector();
    if (channels_[0].receive_interrupt) {
        channels_[0].receive_interrupt = false;
        channels_[0].interrupt_in_service = true;
    } else if (channels_[0].transmit_interrupt) {
        channels_[0].transmit_interrupt = false;
        channels_[0].interrupt_in_service = true;
    } else if (channels_[1].receive_interrupt) {
        channels_[1].receive_interrupt = false;
        channels_[1].interrupt_in_service = true;
    } else if (channels_[1].transmit_interrupt) {
        channels_[1].transmit_interrupt = false;
        channels_[1].interrupt_in_service = true;
    }
    return acknowledged_vector;
}

bool Sio::interrupt_reti() {
    for (auto& channel : channels_) {
        if (!channel.interrupt_in_service) continue;
        channel.interrupt_in_service = false;
        return true;
    }
    return false;
}

} // namespace vz256
