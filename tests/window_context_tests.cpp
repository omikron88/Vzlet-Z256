#include "about_window.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstdio>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void begin_frame() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void check_child_exists() {
    int count{};
    auto** windows = SDL_GetWindows(&count);
    SDL_free(windows);
    require(count == 2, "About window did not open");
}
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    auto* window = SDL_CreateWindow("Window context test", 640, 480, 0);
    auto* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (!renderer) { SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    auto* main_context = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) ||
        !ImGui_ImplSDLRenderer3_Init(renderer)) return 1;

    int result = 0;
    try {
        // Menu actions open the child in the middle of the main UI frame.
        begin_frame();
        {
            AboutWindow about(window);
            about.open();
            check_child_exists();
            require(ImGui::GetCurrentContext() == main_context,
                    "Opening a child changed the current context");
            ImGui::Render();
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
            about.render();
            require(ImGui::GetCurrentContext() == main_context,
                    "Rendering a child changed the current context");

            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                (void)about.handle_event(event);
                require(ImGui::GetCurrentContext() == main_context,
                        "Child event handling changed the current context");
            }

            // Reopening must preserve the context too.
            about.open();
            require(ImGui::GetCurrentContext() == main_context,
                    "Reopening a child changed the current context");
        }
        require(ImGui::GetCurrentContext() == main_context,
                "Destroying a child changed the current context");

        // Keyboard shortcuts open the child before the main frame begins.
        {
            AboutWindow about(window);
            about.open();
            check_child_exists();
            require(ImGui::GetCurrentContext() == main_context,
                    "Opening before NewFrame changed the current context");
            begin_frame();
            ImGui::Render();
            about.render();
        }
        require(ImGui::GetCurrentContext() == main_context,
                "Child lifecycle lost the main context");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        result = 1;
    }
    ImGui::SetCurrentContext(main_context);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(main_context);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
