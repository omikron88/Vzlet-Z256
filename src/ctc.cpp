#include "vz256/ctc.hpp"

#include <algorithm>

namespace vz256 {

void Ctc::reset() {
    channels_ = {};
    for (auto& channel : channels_) channel.prescaler = 16;
    vector_ = 0;
}

void Ctc::write(std::uint8_t channel_number, std::uint8_t value) {
    auto& channel = channels_.at(channel_number & 3U);
    if (channel.expect_constant) {
        channel.expect_constant = false;
        channel.constant = value == 0 ? 256U : value;
        channel.counter = channel.constant;
        channel.prescaler_cycles = 0;
        channel.running = channel.counter_mode || !channel.external_trigger;
        return;
    }
    if ((value & 1U) == 0) {
        if ((channel_number & 3U) == 0) vector_ = static_cast<std::uint8_t>(value & 0xF8U);
        return;
    }
    channel.interrupt_enabled = (value & 0x80U) != 0;
    channel.counter_mode = (value & 0x40U) != 0;
    channel.prescaler = (value & 0x20U) != 0 ? 256U : 16U;
    channel.external_trigger = (value & 0x08U) != 0;
    channel.expect_constant = (value & 0x04U) != 0;
    channel.interrupt_pending = false;
    if ((value & 0x02U) != 0) {
        channel.running = false;
        channel.prescaler_cycles = 0;
    }
}

std::uint8_t Ctc::read(std::uint8_t channel) const {
    return static_cast<std::uint8_t>(channels_.at(channel & 3U).counter & 0xFFU);
}

std::uint32_t Ctc::advance(std::uint8_t channel_number, std::uint32_t ticks) {
    auto& channel = channels_.at(channel_number & 3U);
    if (!channel.running || channel.constant == 0 || ticks == 0) return 0;
    std::uint32_t terminal_counts = 0;
    while (ticks >= channel.counter) {
        ticks -= channel.counter;
        channel.counter = channel.constant;
        ++terminal_counts;
        if (channel.interrupt_enabled) channel.interrupt_pending = true;
    }
    channel.counter = static_cast<std::uint16_t>(channel.counter - ticks);
    return terminal_counts;
}

void Ctc::tick(std::uint32_t cycles) {
    for (std::uint8_t index = 0; index < channels_.size(); ++index) {
        auto& channel = channels_[index];
        if (!channel.running || channel.counter_mode) continue;
        const auto accumulated = channel.prescaler_cycles + cycles;
        const auto ticks = accumulated / channel.prescaler;
        channel.prescaler_cycles = accumulated % channel.prescaler;
        const auto terminal_counts = advance(index, ticks);
        // On the CPU board, channel 2 provides the clock input for channel 3.
        if (index == 2 && terminal_counts != 0) trigger(3, terminal_counts);
    }
}

void Ctc::trigger(std::uint8_t channel_number, std::uint32_t pulses) {
    auto& channel = channels_.at(channel_number & 3U);
    if (channel.counter_mode) {
        (void)advance(channel_number, pulses);
    } else if (channel.external_trigger && !channel.running) {
        channel.running = true;
    }
}

bool Ctc::interrupt_pending() const {
    return std::any_of(channels_.begin(), channels_.end(),
                       [](const Channel& channel) { return channel.interrupt_pending; });
}

std::uint8_t Ctc::interrupt_acknowledge() {
    for (std::uint8_t index = 0; index < channels_.size(); ++index) {
        if (!channels_[index].interrupt_pending) continue;
        channels_[index].interrupt_pending = false;
        return static_cast<std::uint8_t>(vector_ + index * 2U);
    }
    return vector_;
}

} // namespace vz256
