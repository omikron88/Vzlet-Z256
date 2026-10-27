#include "window_layout.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace {
std::optional<std::filesystem::path> layout_path(std::string_view name) {
    char* preference_path = SDL_GetPrefPath("Vzlet", "Vzlet-Z256");
    if (preference_path == nullptr) return std::nullopt;
    const std::filesystem::path directory(preference_path);
    SDL_free(preference_path);
    return directory / (std::string(name) + ".window");
}
}

void restore_window_layout(SDL_Window* window, std::string_view name) {
    const auto path = layout_path(name);
    if (window == nullptr || !path) return;
    int x{}, y{}, width{}, height{};
    std::ifstream input(*path);
    if (!(input >> x >> y >> width >> height) || width < 320 || height < 200) return;
    SDL_SetWindowSize(window, width, height);
    SDL_SetWindowPosition(window, x, y);
}

void save_window_layout(SDL_Window* window, std::string_view name) {
    const auto path = layout_path(name);
    if (window == nullptr || !path) return;
    int x{}, y{}, width{}, height{};
    if (!SDL_GetWindowPosition(window, &x, &y) ||
        !SDL_GetWindowSize(window, &width, &height)) return;
    std::ofstream output(*path, std::ios::trunc);
    if (output) output << x << ' ' << y << ' ' << width << ' ' << height << '\n';
}
