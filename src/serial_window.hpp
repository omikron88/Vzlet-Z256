#pragma once

#include "tcp_serial.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

using SerialLinks = std::array<std::unique_ptr<TcpSerial>, 2>;

class SerialWindow {
public:
    SerialWindow(SDL_Window* parent, SerialLinks& links);
    ~SerialWindow();
    SerialWindow(const SerialWindow&) = delete;
    SerialWindow& operator=(const SerialWindow&) = delete;

    void configure(std::size_t channel, TcpSerial::Mode mode, std::string_view host,
                   std::uint16_t port);
    void open();
    [[nodiscard]] bool handle_event(const SDL_Event& event);
    void render();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
