#pragma once

#include <SDL3/SDL.h>

#include <string_view>

void restore_window_layout(SDL_Window* window, std::string_view name);
void save_window_layout(SDL_Window* window, std::string_view name);
