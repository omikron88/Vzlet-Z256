#include "view_settings.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace {

std::optional<std::filesystem::path> settings_path() {
    char* preference_path = SDL_GetPrefPath("Vzlet", "Vzlet-Z256");
    if (preference_path == nullptr) return std::nullopt;
    const std::filesystem::path directory(preference_path);
    SDL_free(preference_path);
    return directory / "view.cfg";
}

void parse_bool(const std::string& value, bool& result) {
    if (value == "1" || value == "true") result = true;
    else if (value == "0" || value == "false") result = false;
}

} // namespace

ViewSettings load_view_settings() {
    ViewSettings settings;
    const auto path = settings_path();
    if (!path) return settings;

    std::ifstream input(*path);
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const auto name = line.substr(0, separator);
        const auto value = line.substr(separator + 1);
        if (name == "keep_aspect") parse_bool(value, settings.keep_aspect);
        else if (name == "integer_scaling") parse_bool(value, settings.integer_scaling);
        else if (name == "scanlines") parse_bool(value, settings.scanlines);
        else if (name == "fullscreen") parse_bool(value, settings.fullscreen);
    }
    return settings;
}

void save_view_settings(const ViewSettings& settings) {
    const auto path = settings_path();
    if (!path) return;
    std::ofstream output(*path, std::ios::trunc);
    if (!output) return;
    output << "keep_aspect=" << settings.keep_aspect << '\n'
           << "integer_scaling=" << settings.integer_scaling << '\n'
           << "scanlines=" << settings.scanlines << '\n'
           << "fullscreen=" << settings.fullscreen << '\n';
}
