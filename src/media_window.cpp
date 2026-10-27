#include "imgui_context_scope.hpp"
#include "media_window.hpp"
#include "recent_media.hpp"
#include "window_layout.hpp"

#include "vz256/floppy.hpp"
#include "vz256/machine.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <array>
#include <filesystem>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

namespace {
constexpr std::array<std::string_view, 6> geometry_names{
    "auto", "5.25-dsdd-80", "5.25-dsdd-40", "8-sssd-77", "8-dssd-77", "8-dsdd-77"};
}

struct MediaWindow::Impl {
    SDL_Window* parent{};
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    ImGuiContext* context{};
    vz256::Machine& machine;
    RecentMedia& recent_media;
    std::size_t drive{};
    std::size_t geometry{};
    std::size_t picker_drive{};
    std::size_t picker_geometry{};
    std::optional<std::size_t> confirm_eject;
    std::string message;
    std::mutex mutex;
    bool picker_active{};
    bool open{};
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> pending_mount;
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> pending_create;
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> confirm_create;
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> approved_create;

    Impl(SDL_Window* owner, vz256::Machine& emulated_machine, RecentMedia& recent)
        : parent(owner), machine(emulated_machine), recent_media(recent) {}
    ~Impl() { destroy(); }

    void create() {
        const ImGuiContextScope context_scope;
        if (window != nullptr) return;
        window = SDL_CreateWindow("Vzlet Z-256 - Disk drives", 760, 480,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        if (window != nullptr) SDL_SetWindowParent(window, parent);
        restore_window_layout(window, "media");
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
            throw std::runtime_error("Unable to initialize disk drive UI");
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
            save_window_layout(window, "media");
            SDL_DestroyWindow(window); window = nullptr;
        }
    }

    static void SDLCALL selected_image(void* userdata, const char* const* files, int) {
        auto& self = *static_cast<Impl*>(userdata);
        std::scoped_lock lock(self.mutex);
        self.picker_active = false;
        if (files != nullptr && files[0] != nullptr)
            self.pending_mount = std::tuple{self.picker_drive, self.picker_geometry,
                                            std::filesystem::path(files[0])};
    }

    static void SDLCALL selected_new_image(void* userdata, const char* const* files, int) {
        auto& self = *static_cast<Impl*>(userdata);
        std::scoped_lock lock(self.mutex);
        self.picker_active = false;
        if (files != nullptr && files[0] != nullptr)
            self.pending_create = std::tuple{self.picker_drive, self.picker_geometry,
                                             std::filesystem::path(files[0])};
    }

    void pick(bool create_image) {
        static constexpr SDL_DialogFileFilter open_filters[]{{"Disk images", "img;dsk;raw;imd"},
                                                              {"All files", "*"}};
        static constexpr SDL_DialogFileFilter create_filters[]{{"Disk images", "img;dsk;raw"},
                                                                {"All files", "*"}};
        {
            std::scoped_lock lock(mutex);
            if (picker_active) return;
            picker_active = true;
            picker_drive = drive;
            picker_geometry = geometry;
        }
        if (create_image)
            SDL_ShowSaveFileDialog(selected_new_image, this, window, create_filters, 2, nullptr);
        else
            SDL_ShowOpenFileDialog(selected_image, this, window, open_filters, 2, nullptr, false);
    }

    void request_eject() {
        auto& selected = machine.drive(drive);
        if (!selected.mounted()) return;
        if (!machine.media_change_allowed()) message = "Controller is busy";
        else if (selected.dirty()) confirm_eject = drive;
        else { selected.eject(); message = "Image ejected"; }
    }
};

MediaWindow::MediaWindow(SDL_Window* parent, vz256::Machine& machine,
                         RecentMedia& recent_media)
    : impl_(std::make_unique<Impl>(parent, machine, recent_media)) {}
MediaWindow::~MediaWindow() = default;

void MediaWindow::open() {
    try { impl_->create(); }
    catch (const std::exception& error) { SDL_Log("Disk drives: %s", error.what()); return; }
    impl_->open = true;
    impl_->confirm_eject.reset();
    impl_->message.clear();
    SDL_ShowWindow(impl_->window);
    SDL_RaiseWindow(impl_->window);
}

bool MediaWindow::active() const { return impl_->open; }

void MediaWindow::update() {
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> mount;
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> create;
    auto approved_create = std::move(impl_->approved_create);
    impl_->approved_create.reset();
    {
        std::scoped_lock lock(impl_->mutex);
        mount = std::move(impl_->pending_mount);
        impl_->pending_mount.reset();
        create = std::move(impl_->pending_create);
        impl_->pending_create.reset();
    }
    auto apply = [&](const auto& request, bool create_image) {
        if (!request) return;
        const auto& [drive_index, geometry_index, path] = *request;
        auto& selected = impl_->machine.drive(drive_index);
        const auto* geometry = geometry_index == 0
            ? nullptr : vz256::floppy_geometries::find(geometry_names[geometry_index]);
        if (!impl_->machine.media_change_allowed())
            impl_->message = create_image ? "Controller is busy; image was not created"
                                          : "Controller is busy; image was not changed";
        else if (selected.dirty()) impl_->message = "Save or eject the modified image first";
        else if (create_image && geometry == nullptr)
            impl_->message = "Select a geometry before creating an image";
        else {
            vz256::FloppyImage replacement;
            const bool ready = create_image
                ? replacement.create(path, *geometry, 0xe5)
                : geometry != nullptr ? replacement.load(path, *geometry) : replacement.load(path);
            if (ready) {
                selected = std::move(replacement);
                impl_->recent_media.add(RecentMedia::Kind::disk, path);
                impl_->message = create_image ? "Blank 0xE5 image created and mounted"
                                              : "Image mounted";
            } else impl_->message = create_image ? "Unable to create image"
                                                 : "Unknown geometry or invalid image size";
        }
    };
    apply(mount, false);
    if (create && std::filesystem::exists(std::get<2>(*create)))
        impl_->confirm_create = std::move(create);
    else
        apply(create, true);
    apply(approved_create, true);
}

bool MediaWindow::handle_event(const SDL_Event& event) {
    if (impl_->window == nullptr || SDL_GetWindowFromEvent(&event) != impl_->window) return false;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        impl_->open = false;
        SDL_HideWindow(impl_->window);
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
        auto& selected = impl_->machine.drive(impl_->drive);
        if (impl_->confirm_eject) {
            if (event.key.key == SDLK_ESCAPE) impl_->confirm_eject.reset();
            else if (event.key.key == SDLK_D) {
                selected.eject();
                impl_->confirm_eject.reset();
                impl_->message = "Changes discarded; image ejected";
            } else if (event.key.key == SDLK_S) {
                if (selected.save()) {
                    selected.eject();
                    impl_->confirm_eject.reset();
                    impl_->message = "Image saved and ejected";
                } else impl_->message = "Unable to save image";
            }
        } else if (event.key.key == SDLK_UP) impl_->drive = (impl_->drive + 3) % 4;
        else if (event.key.key == SDLK_DOWN) impl_->drive = (impl_->drive + 1) % 4;
        else if (event.key.key == SDLK_LEFT)
            impl_->geometry = (impl_->geometry + geometry_names.size() - 1) % geometry_names.size();
        else if (event.key.key == SDLK_RIGHT)
            impl_->geometry = (impl_->geometry + 1) % geometry_names.size();
        else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_O) {
            if (!impl_->machine.media_change_allowed()) impl_->message = "Controller is busy";
            else if (selected.dirty()) impl_->message = "Save or eject the modified image first";
            else impl_->pick(false);
        } else if (event.key.key == SDLK_N) {
            if (!impl_->machine.media_change_allowed()) impl_->message = "Controller is busy";
            else if (selected.dirty()) impl_->message = "Save or eject the modified image first";
            else if (impl_->geometry == 0) impl_->message = "Select a geometry before creating an image";
            else impl_->pick(true);
        } else if (event.key.key == SDLK_E) impl_->request_eject();
        else if (event.key.key == SDLK_S && selected.dirty())
            impl_->message = selected.save() ? "Image saved" : "Unable to save image";
        else if (event.key.key == SDLK_W && selected.mounted()) {
            const bool protect = !selected.write_protected();
            impl_->message = selected.set_write_protected(protect)
                ? (protect ? "Write protection enabled" : "Write protection disabled")
                : "Image file is not writable";
        }
    }
    return true;
}

void MediaWindow::render() {
    if (!impl_->open) return;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDLRenderer3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Disk drives", nullptr, ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextUnformatted("Select a drive and geometry, then mount or create an image.");
    ImGui::Separator();
    for (std::size_t i = 0; i < 4; ++i) {
        const auto& mounted = impl_->machine.drive(i);
        ImGui::PushID(static_cast<int>(i));
        const std::string label = std::string("Drive ") + static_cast<char>('A' + i);
        if (ImGui::Selectable(label.c_str(), impl_->drive == i,
                              ImGuiSelectableFlags_SpanAllColumns)) impl_->drive = i;
        ImGui::SameLine(105.0F);
        if (mounted.mounted()) {
            std::string details = mounted.path().filename().string() + "  [" +
                                  mounted.geometry().name + "]";
            if (mounted.write_protected()) details += "  read-only";
            if (mounted.dirty()) details += "  modified";
            ImGui::TextUnformatted(details.c_str());
        } else ImGui::TextUnformatted("<empty>");
        ImGui::PopID();
    }
    ImGui::Separator();
    if (ImGui::BeginCombo("Image geometry", geometry_names[impl_->geometry].data())) {
        for (std::size_t i = 0; i < geometry_names.size(); ++i) {
            const bool selected = impl_->geometry == i;
            if (ImGui::Selectable(geometry_names[i].data(), selected)) impl_->geometry = i;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    auto& selected = impl_->machine.drive(impl_->drive);
    const bool change_allowed = impl_->machine.media_change_allowed();
    ImGui::BeginDisabled(!change_allowed || selected.dirty());
    if (ImGui::Button("Open image...")) impl_->pick(false);
    ImGui::SameLine();
    ImGui::BeginDisabled(impl_->geometry == 0);
    if (ImGui::Button("Create blank...")) impl_->pick(true);
    ImGui::EndDisabled(); ImGui::EndDisabled(); ImGui::SameLine();
    ImGui::BeginDisabled(!selected.mounted());
    if (ImGui::Button("Eject")) impl_->request_eject();
    ImGui::SameLine();
    ImGui::BeginDisabled(!selected.dirty());
    if (ImGui::Button("Save")) impl_->message = selected.save() ? "Image saved" : "Unable to save image";
    ImGui::EndDisabled(); ImGui::EndDisabled();
    if (selected.mounted()) {
        bool writable = !selected.write_protected();
        if (ImGui::Checkbox("Writable", &writable) && !selected.set_write_protected(!writable))
            impl_->message = "Image file is not writable";
    }
    const auto& recent = impl_->recent_media.items(RecentMedia::Kind::disk);
    if (!recent.empty()) {
        ImGui::Separator();
        ImGui::TextUnformatted("Recent disk images");
        std::optional<std::filesystem::path> remove_recent;
        for (std::size_t i = 0; i < recent.size(); ++i) {
            const auto& path = recent[i];
            ImGui::PushID(static_cast<int>(i + 100));
            const bool available = std::filesystem::is_regular_file(path);
            ImGui::BeginDisabled(!available || !change_allowed || selected.dirty());
            if (ImGui::Button(path.filename().string().c_str())) {
                std::scoped_lock lock(impl_->mutex);
                impl_->pending_mount = std::tuple{impl_->drive, std::size_t{0}, path};
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", path.string().c_str());
            ImGui::SameLine();
            if (!available) ImGui::TextDisabled("missing");
            else ImGui::TextDisabled("%s", path.parent_path().string().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove")) remove_recent = path;
            ImGui::PopID();
        }
        if (remove_recent) impl_->recent_media.remove(RecentMedia::Kind::disk, *remove_recent);
        if (ImGui::SmallButton("Clear recent disk images"))
            impl_->recent_media.clear(RecentMedia::Kind::disk);
    }
    if (impl_->confirm_create) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0F, 0.65F, 0.2F, 1.0F),
                           "The selected image already exists.");
        ImGui::Text("Replace %s?", std::get<2>(*impl_->confirm_create).filename().string().c_str());
        if (ImGui::Button("Replace image")) {
            impl_->approved_create = std::move(impl_->confirm_create);
            impl_->confirm_create.reset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            impl_->confirm_create.reset();
            impl_->message = "Image creation cancelled";
        }
    } else if (impl_->confirm_eject) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0F, 0.8F, 0.25F, 1.0F),
                           "The image contains unsaved changes.");
        if (ImGui::Button("Save and eject")) {
            if (selected.save()) { selected.eject(); impl_->confirm_eject.reset(); impl_->message = "Image saved and ejected"; }
            else impl_->message = "Unable to save image";
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard and eject")) {
            selected.eject(); impl_->confirm_eject.reset(); impl_->message = "Changes discarded";
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) impl_->confirm_eject.reset();
    } else if (!impl_->message.empty())
        ImGui::TextColored(ImVec4(0.45F, 0.8F, 1.0F, 1.0F), "%s", impl_->message.c_str());
    ImGui::End(); ImGui::Render();
    SDL_SetRenderDrawColor(impl_->renderer, 20, 20, 20, 255); SDL_RenderClear(impl_->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), impl_->renderer);
    SDL_RenderPresent(impl_->renderer);
}
