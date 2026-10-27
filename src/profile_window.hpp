#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>
#include <optional>

enum class ProfileOperation { save, load };

struct ProfileRequest {
    ProfileOperation operation{};
    std::filesystem::path archive;
};

class ProfileWindow {
public:
    ProfileWindow(SDL_Window* parent, std::filesystem::path configuration_directory);
    ~ProfileWindow();
    void open();
    bool handle_event(const SDL_Event& event);
    void render();
    std::optional<ProfileRequest> take_request();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
