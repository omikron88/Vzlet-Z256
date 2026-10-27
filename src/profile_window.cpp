#include "imgui_context_scope.hpp"
#include "profile_window.hpp"

#include "window_layout.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
enum class Confirmation { none, overwrite, load, remove };

bool valid_name(const char* name) {
    if (name[0] == '\0') return false;
    for (const auto* character = name; *character != '\0'; ++character)
        if (!std::isalnum(static_cast<unsigned char>(*character)) && *character != ' ' &&
            *character != '-' && *character != '_') return false;
    return true;
}
}

struct ProfileWindow::Impl {
    SDL_Window* parent{};
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    ImGuiContext* context{};
    std::filesystem::path profiles_directory;
    std::vector<std::filesystem::path> profiles;
    std::array<char, 64> name{};
    int selected{-1};
    bool open{};
    std::optional<ProfileRequest> request;
    Confirmation confirmation{Confirmation::none};
    std::filesystem::path confirmation_path;

    Impl(SDL_Window* owner, std::filesystem::path configuration_directory)
        : parent(owner), profiles_directory(std::move(configuration_directory) / "profiles") {}
    ~Impl() { destroy(); }

    void refresh() {
        profiles.clear();
        std::error_code error;
        std::filesystem::create_directories(profiles_directory, error);
        for (std::filesystem::directory_iterator iterator(profiles_directory, error), end;
             !error && iterator != end; iterator.increment(error)) {
            if (iterator->is_regular_file() && iterator->path().extension() == ".vzconfig")
                profiles.push_back(iterator->path());
        }
        std::sort(profiles.begin(), profiles.end());
        if (selected >= static_cast<int>(profiles.size())) selected = -1;
    }

    void create() {
        const ImGuiContextScope context_scope;
        if (window != nullptr) return;
        window = SDL_CreateWindow("Vzlet Z-256 - Configuration profiles", 560, 430,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        if (window != nullptr) SDL_SetWindowParent(window, parent);
        restore_window_layout(window, "profiles");
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
            throw std::runtime_error("Unable to initialize configuration profiles window");
        }
        refresh();
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
            save_window_layout(window, "profiles");
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }
};

ProfileWindow::ProfileWindow(SDL_Window* parent, std::filesystem::path configuration_directory)
    : impl_(std::make_unique<Impl>(parent, std::move(configuration_directory))) {}
ProfileWindow::~ProfileWindow() = default;

void ProfileWindow::open() {
    try { impl_->create(); }
    catch (const std::exception& error) { SDL_Log("Profiles: %s", error.what()); return; }
    impl_->refresh();
    impl_->open = true;
    SDL_ShowWindow(impl_->window);
    SDL_RaiseWindow(impl_->window);
}

bool ProfileWindow::handle_event(const SDL_Event& event) {
    if (impl_->window == nullptr || SDL_GetWindowFromEvent(&event) != impl_->window) return false;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
        (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
        impl_->open = false;
        SDL_HideWindow(impl_->window);
    }
    return true;
}

void ProfileWindow::render() {
    if (!impl_->open) return;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDLRenderer3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Configuration profiles", nullptr, ImGuiWindowFlags_NoDecoration |
                                                ImGuiWindowFlags_NoMove |
                                                ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextWrapped("Profiles contain all saved view, shortcut and window-layout settings. Saving or loading closes the emulator so the complete state can be transferred safely.");
    ImGui::Separator();
    ImGui::InputTextWithHint("##name", "New profile name", impl_->name.data(), impl_->name.size());
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid_name(impl_->name.data()));
    if (ImGui::Button("Save current & exit")) {
        const auto archive = impl_->profiles_directory /
                             (std::string(impl_->name.data()) + ".vzconfig");
        if (std::filesystem::exists(archive)) {
            impl_->confirmation = Confirmation::overwrite;
            impl_->confirmation_path = archive;
            ImGui::OpenPopup("Confirm profile operation");
        } else impl_->request = ProfileRequest{ProfileOperation::save, archive};
    }
    ImGui::EndDisabled();
    ImGui::Separator();
    ImGui::BeginChild("profiles", ImVec2(0, -70), true);
    for (std::size_t index = 0; index < impl_->profiles.size(); ++index) {
        const auto label = impl_->profiles[index].stem().string();
        if (ImGui::Selectable(label.c_str(), impl_->selected == static_cast<int>(index)))
            impl_->selected = static_cast<int>(index);
    }
    ImGui::EndChild();
    const bool selected = impl_->selected >= 0 &&
                          impl_->selected < static_cast<int>(impl_->profiles.size());
    ImGui::BeginDisabled(!selected);
    if (ImGui::Button("Load & exit", ImVec2(120, 0))) {
        impl_->confirmation = Confirmation::load;
        impl_->confirmation_path = impl_->profiles[static_cast<std::size_t>(impl_->selected)];
        ImGui::OpenPopup("Confirm profile operation");
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete", ImVec2(100, 0))) {
        impl_->confirmation = Confirmation::remove;
        impl_->confirmation_path = impl_->profiles[static_cast<std::size_t>(impl_->selected)];
        ImGui::OpenPopup("Confirm profile operation");
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(100, 0))) {
        impl_->open = false;
        SDL_HideWindow(impl_->window);
    }
    if (ImGui::BeginPopupModal("Confirm profile operation", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        if (impl_->confirmation == Confirmation::overwrite)
            ImGui::Text("Replace profile '%s'?", impl_->confirmation_path.stem().string().c_str());
        else if (impl_->confirmation == Confirmation::load)
            ImGui::Text("Replace the current configuration with profile '%s'?",
                        impl_->confirmation_path.stem().string().c_str());
        else
            ImGui::Text("Permanently delete profile '%s'?",
                        impl_->confirmation_path.stem().string().c_str());
        ImGui::TextUnformatted("This operation cannot be undone.");
        ImGui::Separator();
        if (ImGui::Button("Confirm", ImVec2(110, 0))) {
            if (impl_->confirmation == Confirmation::overwrite)
                impl_->request = ProfileRequest{ProfileOperation::save, impl_->confirmation_path};
            else if (impl_->confirmation == Confirmation::load)
                impl_->request = ProfileRequest{ProfileOperation::load, impl_->confirmation_path};
            else if (impl_->confirmation == Confirmation::remove) {
                std::error_code error;
                std::filesystem::remove(impl_->confirmation_path, error);
                impl_->refresh();
            }
            impl_->confirmation = Confirmation::none;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110, 0))) {
            impl_->confirmation = Confirmation::none;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End(); ImGui::Render();
    SDL_SetRenderDrawColor(impl_->renderer, 20, 20, 20, 255); SDL_RenderClear(impl_->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), impl_->renderer);
    SDL_RenderPresent(impl_->renderer);
}

std::optional<ProfileRequest> ProfileWindow::take_request() {
    return std::exchange(impl_->request, std::nullopt);
}
