#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>

namespace vz256 {
class Machine;
}
class RecentMedia;

class TapeWindow {
public:
    TapeWindow(SDL_Window* parent, vz256::Machine& machine,
               std::filesystem::path& configured_path, RecentMedia& recent_media);
    ~TapeWindow();
    TapeWindow(const TapeWindow&) = delete;
    TapeWindow& operator=(const TapeWindow&) = delete;

    void open();
    void update();
    [[nodiscard]] bool handle_event(const SDL_Event& event);
    void render();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
