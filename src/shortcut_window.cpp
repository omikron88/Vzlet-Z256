#include "imgui_context_scope.hpp"
#include "shortcut_window.hpp"

#include "window_layout.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
constexpr std::array<SDL_Keycode, 12> function_keys{
    SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6,
    SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12};
constexpr std::array<const char*, 12> function_labels{
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"};
constexpr std::array<const char*, static_cast<std::size_t>(ShortcutAction::count)> action_labels{
    "Pause / resume", "Serial links", "Cassette", "Disk drives", "Debugger", "Reset"};
constexpr std::array<int, static_cast<std::size_t>(ShortcutAction::count)> defaults{4, 7, 8, 9, 10, 11};

std::filesystem::path configuration_path() {
    char* preference_path = SDL_GetPrefPath("Vzlet", "Vzlet-Z256");
    if (preference_path == nullptr) return {};
    const std::filesystem::path path = std::filesystem::path(preference_path) / "shortcuts.cfg";
    SDL_free(preference_path);
    return path;
}
}

struct ShortcutWindow::Impl {
    SDL_Window* parent{};
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    ImGuiContext* context{};
    std::array<int, static_cast<std::size_t>(ShortcutAction::count)> shortcuts{defaults};
    bool open{};

    explicit Impl(SDL_Window* owner) : parent(owner) { load(); }
    ~Impl() { save(); destroy(); }

    void load() {
        std::ifstream input(configuration_path());
        auto loaded = shortcuts;
        for (auto& shortcut : loaded)
            if (!(input >> shortcut) || shortcut < 0 || shortcut >= static_cast<int>(function_keys.size()))
                return;
        shortcuts = loaded;
    }

    void save() const {
        std::ofstream output(configuration_path(), std::ios::trunc);
        if (!output) return;
        for (const auto shortcut : shortcuts) output << shortcut << '\n';
    }

    void create() {
        const ImGuiContextScope context_scope;
        if (window != nullptr) return;
        window = SDL_CreateWindow("Vzlet Z-256 - Keyboard shortcuts", 500, 390,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        if (window != nullptr) SDL_SetWindowParent(window, parent);
        restore_window_layout(window, "shortcuts");
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
            throw std::runtime_error("Unable to initialize shortcut settings");
        }
    }

    void destroy() {
        const ImGuiContextScope context_scope;
        if (context != nullptr) {
            ImGui::SetCurrentContext(context);
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext(context);
            context = nullptr;
        }
        if (renderer != nullptr) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
        if (window != nullptr) {
            save_window_layout(window, "shortcuts");
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }
};

ShortcutWindow::ShortcutWindow(SDL_Window* parent) : impl_(std::make_unique<Impl>(parent)) {}
ShortcutWindow::~ShortcutWindow() = default;

SDL_Keycode ShortcutWindow::key(ShortcutAction action) const {
    return function_keys[static_cast<std::size_t>(impl_->shortcuts[static_cast<std::size_t>(action)])];
}

const char* ShortcutWindow::label(ShortcutAction action) const {
    return function_labels[static_cast<std::size_t>(impl_->shortcuts[static_cast<std::size_t>(action)])];
}

void ShortcutWindow::open() {
    try { impl_->create(); }
    catch (const std::exception& error) { SDL_Log("Shortcuts: %s", error.what()); return; }
    impl_->open = true;
    SDL_ShowWindow(impl_->window);
    SDL_RaiseWindow(impl_->window);
}

bool ShortcutWindow::handle_event(const SDL_Event& event) {
    if (impl_->window == nullptr || SDL_GetWindowFromEvent(&event) != impl_->window) return false;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
        (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
        impl_->open = false;
        impl_->save();
        SDL_HideWindow(impl_->window);
    }
    return true;
}

void ShortcutWindow::render() {
    if (!impl_->open) return;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDLRenderer3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Keyboard shortcuts", nullptr, ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextWrapped("Select function keys used by the main emulator window. Changes apply immediately.");
    ImGui::Separator();
    for (std::size_t action = 0; action < impl_->shortcuts.size(); ++action) {
        ImGui::PushID(static_cast<int>(action));
        ImGui::SetNextItemWidth(110);
        const int previous = impl_->shortcuts[action];
        if (ImGui::Combo(action_labels[action], &impl_->shortcuts[action], function_labels.data(),
                         static_cast<int>(function_labels.size()))) {
            for (std::size_t other = 0; other < impl_->shortcuts.size(); ++other) {
                if (other != action && impl_->shortcuts[other] == impl_->shortcuts[action]) {
                    impl_->shortcuts[other] = previous;
                    break;
                }
            }
            impl_->save();
        }
        ImGui::PopID();
    }
    if (ImGui::Button("Restore defaults")) impl_->shortcuts = defaults;
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(100, 0))) {
        impl_->open = false;
        impl_->save();
        SDL_HideWindow(impl_->window);
    }
    ImGui::End(); ImGui::Render();
    SDL_SetRenderDrawColor(impl_->renderer, 20, 20, 20, 255); SDL_RenderClear(impl_->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), impl_->renderer);
    SDL_RenderPresent(impl_->renderer);
}
