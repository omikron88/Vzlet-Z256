#include "keyboard.hpp"
#include "vz256/machine.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string_view>

namespace {
std::optional<std::uint8_t> press(Keyboard& keyboard, SDL_Scancode code,
                                 SDL_Keymod mods = SDL_KMOD_NONE, bool repeat = false) {
    SDL_KeyboardEvent event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.scancode = code;
    event.mod = mods;
    event.repeat = repeat;
    // Deliberately disagree with the physical key (as a host layout may do).
    event.key = SDLK_UNKNOWN;
    return keyboard.handle_event(event);
}

void release(Keyboard& keyboard, SDL_Scancode code) {
    SDL_KeyboardEvent event{};
    event.type = SDL_EVENT_KEY_UP;
    event.scancode = code;
    assert(!keyboard.handle_event(event));
}

void expect(Keyboard& keyboard, SDL_Scancode code, std::uint8_t byte,
            SDL_Keymod mods = SDL_KMOD_NONE) {
    assert(press(keyboard, code, mods) == byte);
    release(keyboard, code);
}
} // namespace

int main() {
    Keyboard us;
    expect(us, SDL_SCANCODE_A, 'a', static_cast<SDL_Keymod>(SDL_KMOD_CAPS | SDL_KMOD_NUM));
    for (int i = 0; i < 26; ++i) {
        const auto code = static_cast<SDL_Scancode>(SDL_SCANCODE_A + i);
        expect(us, code, static_cast<std::uint8_t>('a' + i));
        expect(us, code, static_cast<std::uint8_t>('A' + i), SDL_KMOD_LSHIFT);
        expect(us, code, static_cast<std::uint8_t>(i + 1), SDL_KMOD_RCTRL);
    }
    constexpr std::string_view digits = "1234567890";
    constexpr std::string_view shifted_digits = "!@#$%^&*()";
    for (int i = 0; i < 10; ++i) {
        const auto code = static_cast<SDL_Scancode>(SDL_SCANCODE_1 + i);
        expect(us, code, static_cast<std::uint8_t>(digits[i]));
        expect(us, code, static_cast<std::uint8_t>(shifted_digits[i]), SDL_KMOD_RSHIFT);
    }
    struct Symbol { SDL_Scancode key; std::uint8_t lower, upper; };
    constexpr std::array symbols{
        Symbol{SDL_SCANCODE_GRAVE, '`', '~'}, Symbol{SDL_SCANCODE_MINUS, '-', '_'},
        Symbol{SDL_SCANCODE_EQUALS, '=', '+'}, Symbol{SDL_SCANCODE_LEFTBRACKET, '[', '{'},
        Symbol{SDL_SCANCODE_RIGHTBRACKET, ']', '}'}, Symbol{SDL_SCANCODE_BACKSLASH, '\\', '|'},
        Symbol{SDL_SCANCODE_SEMICOLON, ';', ':'}, Symbol{SDL_SCANCODE_APOSTROPHE, '\'', '"'},
        Symbol{SDL_SCANCODE_COMMA, ',', '<'}, Symbol{SDL_SCANCODE_PERIOD, '.', '>'},
        Symbol{SDL_SCANCODE_SLASH, '/', '?'}, Symbol{SDL_SCANCODE_BACKSPACE, 8, 8},
        Symbol{SDL_SCANCODE_TAB, 9, 9}, Symbol{SDL_SCANCODE_RETURN, 13, 13},
        Symbol{SDL_SCANCODE_ESCAPE, 27, 27}, Symbol{SDL_SCANCODE_SPACE, 32, 32},
        Symbol{SDL_SCANCODE_F1, 0xd0, 0xd0}, Symbol{SDL_SCANCODE_F2, 0xd1, 0xd1},
        Symbol{SDL_SCANCODE_F3, 0xd2, 0xd2}, Symbol{SDL_SCANCODE_F5, 0x80, 0x81},
        Symbol{SDL_SCANCODE_F6, 0x82, 0x83}, Symbol{SDL_SCANCODE_F7, 0x84, 0x85}};
    for (const auto& symbol : symbols) {
        expect(us, symbol.key, symbol.lower);
        expect(us, symbol.key, symbol.upper, SDL_KMOD_LSHIFT);
    }
    assert(!press(us, SDL_SCANCODE_F4));
    assert(!press(us, SDL_SCANCODE_NONUSBACKSLASH)); // Absent from the 101-key converter.
    assert(press(us, SDL_SCANCODE_A, SDL_KMOD_NONE, true) == 'a');
    assert(!press(us, SDL_SCANCODE_CAPSLOCK));
    expect(us, SDL_SCANCODE_A, 'A');
    expect(us, SDL_SCANCODE_A, 'a', SDL_KMOD_SHIFT);
    expect(us, SDL_SCANCODE_1, '!');
    expect(us, SDL_SCANCODE_MINUS, '_');
    assert(!press(us, SDL_SCANCODE_CAPSLOCK, SDL_KMOD_NONE, true));
    expect(us, SDL_SCANCODE_A, 'A'); // Lock autorepeat must not toggle it again.
    release(us, SDL_SCANCODE_CAPSLOCK);
    assert(!press(us, SDL_SCANCODE_CAPSLOCK));
    expect(us, SDL_SCANCODE_A, 'a');
    const auto ctrl_shift = static_cast<SDL_Keymod>(SDL_KMOD_CTRL | SDL_KMOD_SHIFT);
    expect(us, SDL_SCANCODE_2, 0x00, ctrl_shift); // Ctrl+@, including a valid zero byte.
    expect(us, SDL_SCANCODE_LEFTBRACKET, 0x1b, SDL_KMOD_CTRL);
    expect(us, SDL_SCANCODE_BACKSLASH, 0x1c, SDL_KMOD_CTRL);
    expect(us, SDL_SCANCODE_RIGHTBRACKET, 0x1d, SDL_KMOD_CTRL);
    expect(us, SDL_SCANCODE_6, 0x1e, ctrl_shift);
    expect(us, SDL_SCANCODE_MINUS, 0x1f, ctrl_shift);
    expect(us, SDL_SCANCODE_SLASH, '?', ctrl_shift);
    assert(!press(us, SDL_SCANCODE_1, SDL_KMOD_CTRL));
    assert(!press(us, SDL_SCANCODE_RETURN, SDL_KMOD_CTRL));
    expect(us, SDL_SCANCODE_KP_ENTER, 13, SDL_KMOD_CTRL); // EXTEND bypasses Ctrl.

    constexpr std::array navigation{
        Symbol{SDL_SCANCODE_UP, 0xc1, 0xc1}, Symbol{SDL_SCANCODE_DOWN, 0xc2, 0xc2},
        Symbol{SDL_SCANCODE_RIGHT, 0xc3, 0xc3}, Symbol{SDL_SCANCODE_LEFT, 0xc4, 0xc4},
        Symbol{SDL_SCANCODE_HOME, 0x1e, 0x15}, Symbol{SDL_SCANCODE_END, 0x11, 0x0f},
        Symbol{SDL_SCANCODE_INSERT, 0x16, 0}, Symbol{SDL_SCANCODE_DELETE, 0x7f, 0x7f},
        Symbol{SDL_SCANCODE_PAGEUP, 0x12, 0}, Symbol{SDL_SCANCODE_PAGEDOWN, 0x03, 0}};
    for (const auto& key : navigation) {
        expect(us, key.key, key.lower);
        if (key.upper != 0) expect(us, key.key, key.upper, SDL_KMOD_SHIFT);
        else assert(!press(us, key.key, SDL_KMOD_SHIFT));
    }
    assert(!press(us, SDL_SCANCODE_NUMLOCKCLEAR));
    for (const auto& key : navigation) assert(!press(us, key.key));
    for (int i = 0; i < 10; ++i)
        expect(us, static_cast<SDL_Scancode>(SDL_SCANCODE_KP_1 + i),
               static_cast<std::uint8_t>(digits[i]), SDL_KMOD_SHIFT);
    expect(us, SDL_SCANCODE_KP_DIVIDE, '/', SDL_KMOD_SHIFT);
    expect(us, SDL_SCANCODE_KP_MULTIPLY, '*');
    expect(us, SDL_SCANCODE_KP_MINUS, '-');
    expect(us, SDL_SCANCODE_KP_PLUS, '+');
    expect(us, SDL_SCANCODE_KP_PERIOD, '.');
    assert(!press(us, SDL_SCANCODE_NUMLOCKCLEAR, SDL_KMOD_NONE, true));
    assert(!press(us, SDL_SCANCODE_UP));
    release(us, SDL_SCANCODE_NUMLOCKCLEAR);
    assert(!press(us, SDL_SCANCODE_NUMLOCKCLEAR));
    expect(us, SDL_SCANCODE_UP, 0xc1);

    Keyboard cz;
    assert(!cz.czech());
    assert(!press(cz, SDL_SCANCODE_LALT));
    assert(!cz.czech()); // Commit the toggle on release, keeping host chords free.
    assert(!press(cz, SDL_SCANCODE_LALT, SDL_KMOD_NONE, true));
    release(cz, SDL_SCANCODE_LALT);
    assert(cz.czech());
    constexpr std::array<std::uint8_t, 10> czech_digits{
        '+', 0xe5, 0xf3, 0xe3, 0xf2, 0xfa, 0xf9, 0xe1, 0xe9, 0xf7};
    for (int i = 0; i < 10; ++i) {
        const auto code = static_cast<SDL_Scancode>(SDL_SCANCODE_1 + i);
        expect(cz, code, czech_digits[i]);
        expect(cz, code, static_cast<std::uint8_t>(digits[i]), SDL_KMOD_SHIFT);
    }
    // The source's byte values take precedence over misleading table comments.
    expect(cz, SDL_SCANCODE_Y, 'y');
    expect(cz, SDL_SCANCODE_Z, 'z');
    expect(cz, SDL_SCANCODE_SEMICOLON, 0xea);
    expect(cz, SDL_SCANCODE_SEMICOLON, ':', SDL_KMOD_SHIFT);
    expect(cz, SDL_SCANCODE_APOSTROPHE, 0xdf);
    expect(cz, SDL_SCANCODE_BACKSLASH, 0xdd);
    expect(cz, SDL_SCANCODE_GRAVE, 0xde, SDL_KMOD_SHIFT);
    expect(cz, SDL_SCANCODE_F1, 0xf0); // OUTPUT swaps bit 5 for all c0..ff bytes.
    expect(cz, SDL_SCANCODE_UP, 0xe1);
    assert(!press(cz, SDL_SCANCODE_F5));
    assert(!press(cz, SDL_SCANCODE_F6));
    assert(!press(cz, SDL_SCANCODE_F7));

    assert(!press(cz, SDL_SCANCODE_EQUALS)); // Acute dead key.
    expect(cz, SDL_SCANCODE_A, 0xe1);
    expect(cz, SDL_SCANCODE_A, 'a');
    assert(!press(cz, SDL_SCANCODE_EQUALS, SDL_KMOD_SHIFT)); // Caron dead key.
    assert(!press(cz, SDL_SCANCODE_F4)); // An unmapped key does not consume it.
    expect(cz, SDL_SCANCODE_UP, 0xe1); // Nor does an extended key.
    expect(cz, SDL_SCANCODE_C, 0xe3);
    assert(!press(cz, SDL_SCANCODE_EQUALS, SDL_KMOD_SHIFT));
    expect(cz, SDL_SCANCODE_C, 0xc3, SDL_KMOD_SHIFT);
    assert(!press(cz, SDL_SCANCODE_EQUALS));
    expect(cz, SDL_SCANCODE_E, 0xf7, SDL_KMOD_CTRL); // Accent path precedes Ctrl.
    assert(!press(cz, SDL_SCANCODE_EQUALS));
    expect(cz, SDL_SCANCODE_Q, 'q'); // Unsupported accent consumes only the dead key.
    assert(!press(cz, SDL_SCANCODE_CAPSLOCK));
    assert(!press(cz, SDL_SCANCODE_EQUALS));
    expect(cz, SDL_SCANCODE_E, 0xf7); // Accent case follows Shift, not Caps Lock.
    assert(!press(cz, SDL_SCANCODE_CAPSLOCK));
    assert(!press(cz, SDL_SCANCODE_RALT));
    release(cz, SDL_SCANCODE_RALT);
    assert(!cz.czech());
    expect(cz, SDL_SCANCODE_EQUALS, '=');
    assert(!press(cz, SDL_SCANCODE_LALT));
    expect(cz, SDL_SCANCODE_A, 'a', SDL_KMOD_ALT);
    release(cz, SDL_SCANCODE_LALT);
    assert(!cz.czech()); // Alt with another key is a chord, not a layout toggle.
    assert(!press(cz, SDL_SCANCODE_LALT));
    cz.release(); // Alt+Tab / focus loss cancels the pending toggle.
    release(cz, SDL_SCANCODE_LALT);
    assert(!cz.czech());

    Keyboard hex;
    assert(!press(hex, SDL_SCANCODE_SCROLLLOCK));
    assert(!press(hex, SDL_SCANCODE_0));
    expect(hex, SDL_SCANCODE_0, 0x00);
    assert(!press(hex, SDL_SCANCODE_F));
    assert(!press(hex, SDL_SCANCODE_G)); // Invalid digits leave the first nibble intact.
    expect(hex, SDL_SCANCODE_F, 0xff);
    assert(!press(hex, SDL_SCANCODE_1));
    expect(hex, SDL_SCANCODE_UP, 0xc1); // EXTEND bypasses hex input.
    expect(hex, SDL_SCANCODE_A, 0x1a);
    assert(!press(hex, SDL_SCANCODE_2));
    release(hex, SDL_SCANCODE_SCROLLLOCK); // Incomplete pair is cancelled on release.
    expect(hex, SDL_SCANCODE_A, 'a');
    assert(!press(hex, SDL_SCANCODE_SCROLLLOCK));
    assert(!press(hex, SDL_SCANCODE_3));
    hex.release(); // Losing focus must not leave hex input stuck.
    expect(hex, SDL_SCANCODE_A, 'a');
    hex.set_czech(true);
    assert(!press(hex, SDL_SCANCODE_EQUALS));
    hex.release();
    assert(hex.czech());
    expect(hex, SDL_SCANCODE_A, 'a');
    assert(!press(hex, SDL_SCANCODE_SCROLLLOCK));
    assert(!press(hex, SDL_SCANCODE_D));
    expect(hex, SDL_SCANCODE_KP_0, 0xf0); // Raw d0 also passes Czech OUTPUT.
    release(hex, SDL_SCANCODE_SCROLLLOCK);

    // The converter emits logical bytes. Machine::key performs the one and only
    // active-low inversion for the real parallel PIO interface, including KOI-8.
    vz256::Machine machine;
    Keyboard pio_keyboard;
    pio_keyboard.set_czech(true);
    const auto byte = press(pio_keyboard, SDL_SCANCODE_2);
    assert(byte == 0xe5);
    machine.key(*byte);
    assert(machine.input(0xd4) == static_cast<std::uint8_t>(~0xe5));
}
