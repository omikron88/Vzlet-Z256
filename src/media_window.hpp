#pragma once

#include <SDL3/SDL.h>

#include <memory>

namespace vz256 {
class Machine;
}
class RecentMedia;

class MediaWindow {
public:
    MediaWindow(SDL_Window* parent, vz256::Machine& machine, RecentMedia& recent_media);
    ~MediaWindow();
    MediaWindow(const MediaWindow&) = delete;
    MediaWindow& operator=(const MediaWindow&) = delete;

    void open();
    void update();
    [[nodiscard]] bool handle_event(const SDL_Event& event);
    void render();
    [[nodiscard]] bool active() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
