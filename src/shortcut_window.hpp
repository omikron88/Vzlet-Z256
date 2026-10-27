#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <memory>

enum class ShortcutAction : std::size_t {
    pause,
    serial,
    cassette,
    media,
    debugger,
    reset,
    count
};

class ShortcutWindow {
public:
    explicit ShortcutWindow(SDL_Window* parent);
    ~ShortcutWindow();
    ShortcutWindow(const ShortcutWindow&) = delete;
    ShortcutWindow& operator=(const ShortcutWindow&) = delete;

    void open();
    [[nodiscard]] bool handle_event(const SDL_Event& event);
    void render();
    [[nodiscard]] SDL_Keycode key(ShortcutAction action) const;
    [[nodiscard]] const char* label(ShortcutAction action) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
