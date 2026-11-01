#pragma once

#include <SDL3/SDL_events.h>

#include <cstdint>
#include <optional>

// AT keyboard to parallel ASCII/KOI-8 converter used by the real machine.
// Consumes physical key events; host text input must not also reach the PIO.
class Keyboard {
public:
    [[nodiscard]] std::optional<std::uint8_t> handle_event(const SDL_KeyboardEvent& event);
    [[nodiscard]] bool czech() const { return czech_; }
    void set_czech(bool enabled);
    void cancel_layout_toggle() { alt_pending_.reset(); }
    // Release transient input when focus leaves the emulator; keep lock modes.
    void release();

private:
    [[nodiscard]] std::uint8_t output(std::uint8_t byte) const;
    bool czech_{};
    bool caps_{};
    bool num_{};
    bool scroll_{};
    bool acute_{};
    bool caron_{};
    std::optional<std::uint8_t> hex_high_;
    std::optional<SDL_Scancode> alt_pending_;
};
