#pragma once

#include <SDL3/SDL.h>

#include <memory>

class AboutWindow {
public:
    explicit AboutWindow(SDL_Window* parent);
    ~AboutWindow();
    AboutWindow(const AboutWindow&) = delete;
    AboutWindow& operator=(const AboutWindow&) = delete;

    void open();
    [[nodiscard]] bool handle_event(const SDL_Event& event);
    void render();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
