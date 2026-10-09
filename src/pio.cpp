#include "vz256/pio.hpp"

#include <algorithm>

namespace vz256 {

void Pio::reset() {
    channels_ = {};
    for (auto& channel : channels_) {
        channel.input = 0xff;
        channel.direction = 0xff;
        channel.interrupt_mask = 0xff;
        channel.mode = 1;
    }
}

std::uint8_t Pio::read(std::uint8_t channel_number) {
    auto& channel = channels_.at(channel_number & 1U);
    if (channel.mode == 1U || channel.mode == 2U) {
        channel.interrupt_pending = false;
        return channel.input;
    }
    if (channel.mode == 3U)
        return static_cast<std::uint8_t>((channel.input & channel.direction) |
                                         (channel.output & ~channel.direction));
    return channel.output;
}

void Pio::write(std::uint8_t channel_number, std::uint8_t value) {
    channels_.at(channel_number & 1U).output = value;
}

void Pio::control(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 1U);
    if (channel.expect_direction) {
        channel.direction = value;
        channel.expect_direction = false;
        update_level_interrupt(channel);
    } else if (channel.expect_interrupt_mask) {
        channel.interrupt_mask = value;
        channel.expect_interrupt_mask = false;
        update_level_interrupt(channel);
    } else if ((value & 1U) == 0) {
        channel.vector = value;
    } else if ((value & 0x0fU) == 0x0fU) {
        channel.mode = static_cast<std::uint8_t>((value >> 6U) & 3U);
        channel.expect_direction = channel.mode == 3U;
        channel.interrupt_pending = false;
    } else if ((value & 0x0fU) == 0x07U) {
        channel.interrupt_enabled = (value & 0x80U) != 0;
        channel.interrupt_and = (value & 0x40U) != 0;
        channel.interrupt_high = (value & 0x20U) != 0;
        channel.expect_interrupt_mask = (value & 0x10U) != 0;
        if (!channel.expect_interrupt_mask) update_level_interrupt(channel);
    } else if ((value & 0x0fU) == 0x03U) {
        channel.interrupt_enabled = (value & 0x80U) != 0;
        if (!channel.interrupt_enabled) channel.interrupt_pending = false;
    }
}

void Pio::set_input(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 1U);
    channel.input = value;
    update_level_interrupt(channel);
}

void Pio::strobe(std::uint8_t channel_number) {
    auto& channel = channels_.at(channel_number & 1U);
    if (channel.interrupt_enabled && (channel.mode == 1U || channel.mode == 2U))
        channel.interrupt_pending = true;
}

std::uint8_t Pio::output(std::uint8_t channel) const {
    return channels_.at(channel & 1U).output;
}

void Pio::update_level_interrupt(Channel& channel) {
    if (!channel.interrupt_enabled || channel.mode != 3U) return;
    const auto monitored = static_cast<std::uint8_t>(~channel.interrupt_mask);
    const auto active = channel.interrupt_high ? channel.input
                                                : static_cast<std::uint8_t>(~channel.input);
    const auto matching = static_cast<std::uint8_t>(active & monitored);
    channel.interrupt_pending = monitored != 0U &&
        (channel.interrupt_and ? matching == monitored : matching != 0U);
}

bool Pio::interrupt_pending() const {
    return std::any_of(channels_.begin(), channels_.end(),
                       [](const Channel& channel) { return channel.interrupt_pending; });
}

std::uint8_t Pio::interrupt_acknowledge() {
    for (auto& channel : channels_) {
        if (!channel.interrupt_pending) continue;
        channel.interrupt_pending = false;
        return channel.vector;
    }
    return 0xff;
}

} // namespace vz256
