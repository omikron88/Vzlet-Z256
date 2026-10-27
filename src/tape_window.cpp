#include "imgui_context_scope.hpp"
#include "tape_window.hpp"
#include "recent_media.hpp"
#include "window_layout.hpp"

#include "vz256/machine.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

struct TapeWindow::Impl {
    SDL_Window* parent{};
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    ImGuiContext* context{};
    vz256::Machine& machine;
    std::filesystem::path& configured_path;
    RecentMedia& recent_media;
    std::mutex mutex;
    std::optional<std::filesystem::path> pending_path;
    std::string message;
    bool picker_active{};
    bool create_recording{};
    bool open{};

    Impl(SDL_Window* owner, vz256::Machine& emulated_machine,
         std::filesystem::path& tape_path, RecentMedia& recent)
        : parent(owner), machine(emulated_machine), configured_path(tape_path),
          recent_media(recent) {}
    ~Impl() { destroy(); }

    void create() {
        const ImGuiContextScope context_scope;
        if (window != nullptr) return;
        window = SDL_CreateWindow("Vzlet Z-256 - Cassette", 600, 390,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        if (window != nullptr) SDL_SetWindowParent(window, parent);
        restore_window_layout(window, "cassette");
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
            throw std::runtime_error("Unable to initialize cassette UI");
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
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window != nullptr) {
            save_window_layout(window, "cassette");
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }

    static void SDLCALL selected_file(void* userdata, const char* const* files, int) {
        auto& self = *static_cast<Impl*>(userdata);
        std::scoped_lock lock(self.mutex);
        self.picker_active = false;
        if (files != nullptr && files[0] != nullptr)
            self.pending_path = std::filesystem::path(files[0]);
    }

    void pick_file(bool create) {
        static constexpr SDL_DialogFileFilter filters[]{{"PCM WAV cassette", "wav"},
                                                         {"All files", "*"}};
        {
            std::scoped_lock lock(mutex);
            if (picker_active) return;
            picker_active = true;
            create_recording = create;
        }
        if (create)
            SDL_ShowSaveFileDialog(selected_file, this, window, filters, 2, nullptr);
        else
            SDL_ShowOpenFileDialog(selected_file, this, window, filters, 2, nullptr, false);
    }
};

TapeWindow::TapeWindow(SDL_Window* parent, vz256::Machine& machine,
                       std::filesystem::path& configured_path, RecentMedia& recent_media)
    : impl_(std::make_unique<Impl>(parent, machine, configured_path, recent_media)) {}
TapeWindow::~TapeWindow() = default;

void TapeWindow::open() {
    try {
        impl_->create();
    } catch (const std::exception& error) {
        SDL_Log("Cassette: %s", error.what());
        return;
    }
    impl_->open = true;
    SDL_ShowWindow(impl_->window);
    SDL_RaiseWindow(impl_->window);
}

void TapeWindow::update() {
    std::optional<std::filesystem::path> path;
    bool create{};
    {
        std::scoped_lock lock(impl_->mutex);
        path = std::move(impl_->pending_path);
        impl_->pending_path.reset();
        create = impl_->create_recording;
    }
    if (!path) return;

    auto& tape = impl_->machine.cassette();
    if (tape.recording() && !tape.save_wav()) {
        impl_->message = "Unable to save current recording";
        return;
    }
    const bool ready = create ? tape.create_wav(*path) : tape.load_wav(*path);
    if (ready) {
        impl_->configured_path = *path;
        impl_->recent_media.add(RecentMedia::Kind::tape, *path);
        impl_->message = create ? "Recording created" : "Tape loaded";
    } else {
        impl_->message = "Unable to open cassette WAV";
    }
}

bool TapeWindow::handle_event(const SDL_Event& event) {
    if (impl_->window == nullptr || SDL_GetWindowFromEvent(&event) != impl_->window) return false;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        impl_->open = false;
        SDL_HideWindow(impl_->window);
    }
    return true;
}

void TapeWindow::render() {
    if (!impl_->open) return;
    auto& tape = impl_->machine.cassette();
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Cassette", nullptr, ImGuiWindowFlags_NoDecoration |
                                      ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("Tape: %s", tape.loaded() ? tape.path().filename().string().c_str() : "<empty>");
    ImGui::Text("Mode: %s   Motor: %s", tape.recording() ? "record" : "playback",
                tape.motor() ? "running" : "stopped");
    ImGui::Text("Position: %.1f / %.1f s", tape.position_seconds(), tape.duration_seconds());
    const auto duration = tape.duration_seconds();
    float position = static_cast<float>(tape.position_seconds());
    ImGui::BeginDisabled(!tape.loaded() || duration == 0.0);
    if (ImGui::SliderFloat("##tape-position", &position, 0.0F,
                           static_cast<float>(duration), "%.1f s"))
        tape.seek_seconds(position);
    if (ImGui::Button("|<")) tape.rewind();
    ImGui::SameLine();
    if (ImGui::Button("-10 s")) tape.seek_seconds(tape.position_seconds() - 10.0);
    ImGui::SameLine();
    if (ImGui::Button("+10 s")) tape.seek_seconds(tape.position_seconds() + 10.0);
    ImGui::SameLine();
    if (ImGui::Button(">|")) tape.seek_seconds(duration);
    ImGui::EndDisabled();
    float speed = tape.playback_speed();
    ImGui::BeginDisabled(!tape.loaded() || tape.recording());
    ImGui::SetNextItemWidth(300.0F);
    if (ImGui::SliderFloat("Playback speed", &speed, 0.25F, 4.0F, "%.2fx",
                           ImGuiSliderFlags_Logarithmic))
        tape.set_playback_speed(speed);
    ImGui::SameLine();
    if (ImGui::Button("1x")) tape.set_playback_speed(1.0F);
    ImGui::EndDisabled();
    const auto levels = tape.take_peak_levels();
    ImGui::TextUnformatted("Track levels");
    ImGui::SameLine();
    ImGui::ProgressBar(levels.left, ImVec2(150.0F, 0.0F), "L");
    ImGui::SameLine();
    ImGui::ProgressBar(levels.right, ImVec2(150.0F, 0.0F), "R");
    if (ImGui::Button("Open WAV...")) impl_->pick_file(false);
    ImGui::SameLine();
    if (ImGui::Button("New recording...")) impl_->pick_file(true);
    ImGui::SameLine();
    ImGui::BeginDisabled(!tape.loaded());
    if (ImGui::Button("Eject"))
        impl_->message = tape.eject() ? "Tape ejected" : "Unable to save recording";
    ImGui::EndDisabled();
    const auto& recent = impl_->recent_media.items(RecentMedia::Kind::tape);
    if (!recent.empty()) {
        ImGui::Separator();
        ImGui::TextUnformatted("Recent cassettes");
        std::optional<std::filesystem::path> open_recent;
        std::optional<std::filesystem::path> remove_recent;
        for (std::size_t i = 0; i < recent.size(); ++i) {
            const auto& path = recent[i];
            ImGui::PushID(static_cast<int>(i + 100));
            const bool available = std::filesystem::is_regular_file(path);
            ImGui::BeginDisabled(!available);
            if (ImGui::Button(path.filename().string().c_str())) open_recent = path;
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", path.string().c_str());
            ImGui::SameLine();
            if (!available) ImGui::TextDisabled("missing");
            else ImGui::TextDisabled("%s", path.parent_path().string().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) remove_recent = path;
            ImGui::PopID();
        }
        if (open_recent) {
            std::scoped_lock lock(impl_->mutex);
            impl_->create_recording = false;
            impl_->pending_path = std::move(open_recent);
        }
        if (remove_recent) impl_->recent_media.remove(RecentMedia::Kind::tape, *remove_recent);
        if (ImGui::SmallButton("Clear recent cassettes"))
            impl_->recent_media.clear(RecentMedia::Kind::tape);
    }
    bool monitor = tape.monitor_enabled();
    if (ImGui::Checkbox("Audio monitor", &monitor)) tape.set_monitor_enabled(monitor);
    if (!impl_->message.empty())
        ImGui::TextColored(ImVec4(0.45F, 0.8F, 1.0F, 1.0F), "%s", impl_->message.c_str());
    ImGui::TextWrapped("The emulated SIO RTS signals control the tape motor and playback/recording.");
    ImGui::End();
    ImGui::Render();
    SDL_SetRenderDrawColor(impl_->renderer, 20, 20, 20, 255);
    SDL_RenderClear(impl_->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), impl_->renderer);
    SDL_RenderPresent(impl_->renderer);
}
