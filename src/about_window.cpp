#include "about_window.hpp"

#include "window_layout.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <stdexcept>

#ifndef VZ256_VERSION
#define VZ256_VERSION "development"
#endif

#ifndef VZ256_BUILD_DATE
#define VZ256_BUILD_DATE "unknown"
#endif

struct AboutWindow::Impl {
    SDL_Window* parent{};
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    ImGuiContext* context{};
    bool open{};

    explicit Impl(SDL_Window* owner) : parent(owner) {}
    ~Impl() { destroy(); }

    void create() {
        if (window != nullptr) return;
        window = SDL_CreateWindow("About Vzlet Z-256", 520, 360,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        if (window != nullptr) SDL_SetWindowParent(window, parent);
        restore_window_layout(window, "about");
        renderer = window != nullptr ? SDL_CreateRenderer(window, nullptr) : nullptr;
        if (window == nullptr || renderer == nullptr) {
            destroy();
            throw std::runtime_error(SDL_GetError());
        }
        context = ImGui::CreateContext();
        ImGui::SetCurrentContext(context);
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) ||
            !ImGui_ImplSDLRenderer3_Init(renderer)) {
            destroy();
            throw std::runtime_error("Unable to initialize About window");
        }
    }

    void destroy() {
        if (context != nullptr) {
            ImGui::SetCurrentContext(context);
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext(context);
            context = nullptr;
        }
        if (renderer != nullptr) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
        if (window != nullptr) {
            save_window_layout(window, "about");
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }
};

AboutWindow::AboutWindow(SDL_Window* parent) : impl_(std::make_unique<Impl>(parent)) {}
AboutWindow::~AboutWindow() = default;

void AboutWindow::open() {
    try { impl_->create(); }
    catch (const std::exception& error) { SDL_Log("About: %s", error.what()); return; }
    impl_->open = true;
    SDL_ShowWindow(impl_->window);
    SDL_RaiseWindow(impl_->window);
}

bool AboutWindow::handle_event(const SDL_Event& event) {
    if (impl_->window == nullptr || SDL_GetWindowFromEvent(&event) != impl_->window) return false;
    ImGui::SetCurrentContext(impl_->context);
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
        (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
        impl_->open = false;
        SDL_HideWindow(impl_->window);
    }
    return true;
}

void AboutWindow::render() {
    if (!impl_->open) return;
    ImGui::SetCurrentContext(impl_->context);
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("About", nullptr, ImGuiWindowFlags_NoDecoration |
                                  ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(1.35F);
    ImGui::TextUnformatted("Vzlet Z-256 Emulator");
    ImGui::SetWindowFontScale(1.0F);
    ImGui::Separator();
    ImGui::Text("Version: %s", VZ256_VERSION);
    ImGui::Text("Build date: %s (UTC)", VZ256_BUILD_DATE);
    ImGui::Spacing();
    ImGui::TextWrapped("Emulator of the Czechoslovak Vzlet Z-256 computer.");
    ImGui::TextWrapped("Built with C++20, SDL3, Dear ImGui and redcode/Z80.");
    ImGui::Spacing();
    ImGui::TextUnformatted("Project source:");
    if (ImGui::Button("github.com/omikron88/Vzlet-Z256"))
        SDL_OpenURL("https://github.com/omikron88/Vzlet-Z256");
    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2(100, 0))) {
        impl_->open = false;
        SDL_HideWindow(impl_->window);
    }
    ImGui::End();
    ImGui::Render();
    SDL_SetRenderDrawColor(impl_->renderer, 20, 20, 20, 255);
    SDL_RenderClear(impl_->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), impl_->renderer);
    SDL_RenderPresent(impl_->renderer);
}
