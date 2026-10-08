#include "vz256/cpu.hpp"
#ifdef VZ256_EMBEDDED_ROMS
#include "vz256/embedded_resources.hpp"
#endif
#include "vz256/machine.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

struct Options {
    std::filesystem::path resources{"."};
    std::array<std::filesystem::path, 4> images{};
    std::array<const vz256::FloppyGeometry*, 4> geometries{};
    std::array<bool, 4> image_set{};
    std::array<bool, 4> read_only{};
    std::filesystem::path printer;
};

constexpr std::array<std::string_view, 6> geometry_names{
    "auto", "5.25-dsdd-80", "5.25-dsdd-40", "8-sssd-77", "8-dssd-77", "8-dsdd-77"};

struct MediaDialog {
    bool open{};
    std::size_t drive{};
    std::size_t geometry{};
    std::size_t picker_drive{};
    std::size_t picker_geometry{};
    std::optional<std::size_t> confirm_eject;
    std::string message;
    std::mutex pending_mutex;
    bool picker_active{};
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> pending_mount;
    std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> pending_create;
};

struct UiActions {
    bool reset{};
    bool quit{};
    bool toggle_pause{};
    bool toggle_fullscreen{};
    bool select_printer{};
    bool disconnect_printer{};
};

struct UiState {
    bool keep_aspect{true};
    bool integer_scaling{};
    bool scanlines{};
};

struct PrinterDialog {
    std::mutex mutex;
    bool picker_active{};
    std::optional<std::filesystem::path> pending_path;
};

void SDLCALL selected_printer_file(void* userdata, const char* const* files, int) {
    auto& dialog = *static_cast<PrinterDialog*>(userdata);
    std::scoped_lock lock(dialog.mutex);
    dialog.picker_active = false;
    if (files != nullptr && files[0] != nullptr)
        dialog.pending_path = std::filesystem::path(files[0]);
}

void open_printer_picker(SDL_Window* window, PrinterDialog& dialog) {
    static constexpr SDL_DialogFileFilter filters[]{{"Printer output", "prn;txt"},
                                                     {"All files", "*"}};
    {
        std::scoped_lock lock(dialog.mutex);
        if (dialog.picker_active) return;
        dialog.picker_active = true;
    }
    SDL_ShowSaveFileDialog(selected_printer_file, &dialog, window, filters, 2, nullptr);
}

void SDLCALL selected_image(void* userdata, const char* const* files, int) {
    auto& dialog = *static_cast<MediaDialog*>(userdata);
    std::scoped_lock lock(dialog.pending_mutex);
    dialog.picker_active = false;
    if (files == nullptr || files[0] == nullptr) return;
    dialog.pending_mount = std::tuple{dialog.picker_drive, dialog.picker_geometry,
                                      std::filesystem::path(files[0])};
}

void open_image_picker(SDL_Window* window, MediaDialog& dialog) {
    static constexpr SDL_DialogFileFilter filters[]{{"Disk images", "img;dsk;raw"},
                                                     {"All files", "*"}};
    {
        std::scoped_lock lock(dialog.pending_mutex);
        if (dialog.picker_active) return;
        dialog.picker_active = true;
        dialog.picker_drive = dialog.drive;
        dialog.picker_geometry = dialog.geometry;
    }
    SDL_ShowOpenFileDialog(selected_image, &dialog, window, filters, 2, nullptr, false);
}

void SDLCALL selected_new_image(void* userdata, const char* const* files, int) {
    auto& dialog = *static_cast<MediaDialog*>(userdata);
    std::scoped_lock lock(dialog.pending_mutex);
    dialog.picker_active = false;
    if (files == nullptr || files[0] == nullptr) return;
    dialog.pending_create = std::tuple{dialog.picker_drive, dialog.picker_geometry,
                                       std::filesystem::path(files[0])};
}

void open_new_image_picker(SDL_Window* window, MediaDialog& dialog) {
    static constexpr SDL_DialogFileFilter filters[]{{"Disk images", "img;dsk;raw"},
                                                     {"All files", "*"}};
    {
        std::scoped_lock lock(dialog.pending_mutex);
        if (dialog.picker_active) return;
        dialog.picker_active = true;
        dialog.picker_drive = dialog.drive;
        dialog.picker_geometry = dialog.geometry;
    }
    SDL_ShowSaveFileDialog(selected_new_image, &dialog, window, filters, 2, nullptr);
}

void draw_media_dialog(SDL_Window* window, vz256::Machine& machine, MediaDialog& dialog) {
    if (!dialog.open) return;
    ImGui::SetNextWindowSize(ImVec2(720, 430), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Disk drives", &dialog.open)) {
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted("Select a drive and geometry, then mount or create an image.");
    ImGui::Separator();
    for (std::size_t i = 0; i < 4; ++i) {
        const auto& drive = machine.drive(i);
        ImGui::PushID(static_cast<int>(i));
        const std::string label = std::string("Drive ") + static_cast<char>('A' + i);
        if (ImGui::Selectable(label.c_str(), dialog.drive == i,
                              ImGuiSelectableFlags_SpanAllColumns)) dialog.drive = i;
        ImGui::SameLine(105.0F);
        if (drive.mounted()) {
            std::string details = drive.path().filename().string() + "  [" +
                                  drive.geometry().name + "]";
            if (drive.write_protected()) details += "  read-only";
            if (drive.dirty()) details += "  modified";
            ImGui::TextUnformatted(details.c_str());
        } else ImGui::TextUnformatted("<empty>");
        ImGui::PopID();
    }
    ImGui::Separator();
    if (ImGui::BeginCombo("Image geometry", geometry_names[dialog.geometry].data())) {
        for (std::size_t i = 0; i < geometry_names.size(); ++i) {
            const bool selected = dialog.geometry == i;
            if (ImGui::Selectable(geometry_names[i].data(), selected)) dialog.geometry = i;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    auto& drive = machine.drive(dialog.drive);
    const bool change_allowed = machine.media_change_allowed();
    ImGui::BeginDisabled(!change_allowed || drive.dirty());
    if (ImGui::Button("Open image...")) open_image_picker(window, dialog);
    ImGui::SameLine();
    ImGui::BeginDisabled(dialog.geometry == 0);
    if (ImGui::Button("Create blank...")) open_new_image_picker(window, dialog);
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!drive.mounted());
    if (ImGui::Button("Eject")) {
        if (drive.dirty()) dialog.confirm_eject = dialog.drive;
        else if (change_allowed) { drive.eject(); dialog.message = "Image ejected"; }
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!drive.dirty());
    if (ImGui::Button("Save")) dialog.message = drive.save() ? "Image saved" : "Unable to save image";
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (drive.mounted()) {
        bool writable = !drive.write_protected();
        if (ImGui::Checkbox("Writable", &writable) &&
            !drive.set_write_protected(!writable)) dialog.message = "Image file is not writable";
    }
    if (dialog.confirm_eject) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0F, 0.8F, 0.25F, 1.0F),
                           "The image contains unsaved changes.");
        if (ImGui::Button("Save and eject")) {
            if (drive.save()) { drive.eject(); dialog.confirm_eject.reset(); dialog.message = "Image saved and ejected"; }
            else dialog.message = "Unable to save image";
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard and eject")) {
            drive.eject(); dialog.confirm_eject.reset(); dialog.message = "Changes discarded";
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) dialog.confirm_eject.reset();
    } else if (!dialog.message.empty()) {
        ImGui::TextColored(ImVec4(0.45F, 0.8F, 1.0F, 1.0F), "%s", dialog.message.c_str());
    }
    ImGui::End();
}

UiActions draw_main_ui(vz256::Machine& machine, MediaDialog& dialog, bool paused,
                       bool fullscreen, UiState& state,
                       const std::filesystem::path& printer_path) {
    UiActions actions;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Disk drives", "F10")) dialog.open = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Printer output...")) actions.select_printer = true;
            ImGui::BeginDisabled(printer_path.empty());
            if (ImGui::MenuItem("Disconnect printer")) actions.disconnect_printer = true;
            ImGui::EndDisabled();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) actions.quit = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Control")) {
            if (ImGui::MenuItem("Reset", "F12")) actions.reset = true;
            if (ImGui::MenuItem(paused ? "Resume" : "Pause", "F5")) actions.toggle_pause = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Fullscreen", "Alt+Enter", fullscreen)) actions.toggle_fullscreen = true;
            ImGui::Separator();
            ImGui::MenuItem("Keep 4:3 aspect", nullptr, &state.keep_aspect);
            ImGui::MenuItem("Integer scaling", nullptr, &state.integer_scaling);
            ImGui::MenuItem("Scanlines", nullptr, &state.scanlines);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Vzlet Z-256")) ImGui::OpenPopup("About Vzlet Z-256");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (ImGui::BeginPopupModal("About Vzlet Z-256", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Vzlet Z-256 emulator\nC++20, SDL3, Dear ImGui and redcode/Z80");
        if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    constexpr float status_height = 28.0F;
    const auto display_size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0.0F, std::max(0.0F, display_size.y - status_height)),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(display_size.x, status_height), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(1.0F);
    ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextUnformatted(paused ? "PAUSED" : "RUN  4 MHz");
    for (std::size_t i = 0; i < 4; ++i) {
        ImGui::SameLine();
        const auto& drive = machine.drive(i);
        ImGui::Text("%c:%s%s", static_cast<char>('A' + i),
                    drive.mounted() ? drive.path().filename().string().c_str() : "empty",
                    drive.dirty() ? "*" : "");
    }
    ImGui::SameLine();
    if (printer_path.empty()) ImGui::TextUnformatted("PRN:offline");
    else ImGui::Text("PRN:%s", printer_path.filename().string().c_str());
    ImGui::End();
    return actions;
}

std::optional<std::size_t> drive_option(std::string_view option, std::string_view prefix) {
    if (!option.starts_with(prefix) || option.size() != prefix.size() + 1) return std::nullopt;
    const char letter = option.back();
    return letter >= 'a' && letter <= 'd'
               ? std::optional<std::size_t>{static_cast<std::size_t>(letter - 'a')}
               : std::nullopt;
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view option = argv[i];
        if (option == "--resources" && i + 1 < argc) options.resources = argv[++i];
        else if (option == "--printer" && i + 1 < argc) options.printer = argv[++i];
        else if (const auto drive = drive_option(option, "--drive-"); drive && i + 1 < argc) {
            options.images[*drive] = argv[++i];
            options.image_set[*drive] = true;
        } else if (const auto drive = drive_option(option, "--geometry-"); drive && i + 1 < argc) {
            options.geometries[*drive] = vz256::floppy_geometries::find(argv[++i]);
            if (options.geometries[*drive] == nullptr) return false;
        } else if (const auto drive = drive_option(option, "--read-only-"); drive) {
            options.read_only[*drive] = true;
        } else return false;
    }
    return true;
}

std::optional<std::uint8_t> special_key(const SDL_KeyboardEvent& event) {
    const bool shift = (event.mod & SDL_KMOD_SHIFT) != 0;
    if ((event.mod & SDL_KMOD_CTRL) != 0 && event.key >= SDLK_A && event.key <= SDLK_Z)
        return static_cast<std::uint8_t>(event.key & 0x1f);
    switch (event.key) {
    case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0d;
    case SDLK_TAB: return 0x09;
    case SDLK_ESCAPE: return 0x1b;
    case SDLK_BACKSPACE: return 0x7f;
    case SDLK_UP: return 0xc1;
    case SDLK_DOWN: return 0xc2;
    case SDLK_RIGHT: return 0xc3;
    case SDLK_LEFT: return 0xc4;
    case SDLK_HOME: return 0x8d;
    case SDLK_DELETE: return shift ? 0x81 : 0x80; // ROL
    case SDLK_INSERT: return shift ? 0x83 : 0x82; // COPY
    case SDLK_END: return shift ? 0x85 : 0x84; // BREAK
    case SDLK_F1: return 0xd0;
    case SDLK_F2: return 0xd1;
    case SDLK_F3: return 0xd2;
    default: return std::nullopt;
    }
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, options)) {
        std::cerr << "Usage: vz256 [--resources DIR] [--drive-a IMAGE] "
                     "[--geometry-a PROFILE] [--read-only-a] [--printer FILE] (a..d)\n";
        return 2;
    }

    vz256::Machine machine;
#ifdef VZ256_EMBEDDED_ROMS
    if (!machine.load_roms(vz256::embedded_monitor_rom(),
                           vz256::embedded_character_rom())) {
        std::cerr << "Unable to load embedded ROMs\n";
        return 1;
    }
#else
    if (!machine.load_roms(options.resources / "roms/boot.rom",
                           options.resources / "roms/char.rom")) {
        std::cerr << "Unable to load boot and character ROMs from " << options.resources << '\n';
        return 1;
    }
#endif
#ifdef VZ256_EMBEDDED_ROMS
    if (!options.image_set[0]) {
        if (!machine.drive(0).load(vz256::embedded_boot_disk(),
                                   vz256::floppy_geometries::five_25_dsdd_80)) {
            std::cerr << "Unable to load embedded boot disk\n";
            return 1;
        }
    }
#endif
    if (!options.image_set[0] && std::filesystem::exists(options.resources / "disks/boot.img")) {
        options.images[0] = options.resources / "disks/boot.img";
        options.image_set[0] = true;
    }
    for (std::size_t i = 0; i < options.images.size(); ++i) {
        if (!options.image_set[i]) continue;
        const bool loaded = options.geometries[i] != nullptr
                                ? machine.drive(i).load(options.images[i], *options.geometries[i],
                                                        options.read_only[i])
                                : machine.drive(i).load(options.images[i], options.read_only[i]);
        if (!loaded) {
            std::cerr << "Unable to mount drive " << static_cast<char>('A' + i)
                      << ": unknown geometry or invalid image size\n";
            return 1;
        }
    }

    std::ofstream printer;
    if (!options.printer.empty()) {
        printer.open(options.printer, std::ios::binary | std::ios::app);
        if (!printer) {
            std::cerr << "Unable to open printer output " << options.printer << '\n';
            return 1;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("Vzlet Z-256", 960, 720, SDL_WINDOW_RESIZABLE), SDL_DestroyWindow);
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
        window ? SDL_CreateRenderer(window.get(), nullptr) : nullptr, SDL_DestroyRenderer);
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
        renderer ? SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     vz256::Video::width, vz256::Video::height) : nullptr,
        SDL_DestroyTexture);
    if (!window || !renderer || !texture) {
        std::cerr << "SDL window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
    SDL_StartTextInput(window.get());
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get()) ||
        !ImGui_ImplSDLRenderer3_Init(renderer.get())) {
        std::cerr << "Dear ImGui initialization failed\n";
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_Quit();
        return 1;
    }

    vz256::RedcodeCpu cpu(machine);
    machine.reset();
    cpu.reset();
    std::vector<std::uint32_t> pixels(vz256::Video::width * vz256::Video::height);
    MediaDialog media_dialog;
    PrinterDialog printer_dialog;
    UiState ui_state;
    bool running = true;
    bool paused = false;
    bool fullscreen = false;
    auto previous = std::chrono::steady_clock::now();
    std::uint64_t cycle_fraction = 0;
    while (running) {
        std::optional<std::filesystem::path> pending_printer;
        {
            std::scoped_lock lock(printer_dialog.mutex);
            pending_printer = std::move(printer_dialog.pending_path);
            printer_dialog.pending_path.reset();
        }
        if (pending_printer) {
            std::ofstream replacement(*pending_printer, std::ios::binary | std::ios::app);
            if (replacement) {
                printer = std::move(replacement);
                options.printer = std::move(*pending_printer);
            } else {
                std::cerr << "Unable to open printer output " << *pending_printer << '\n';
            }
        }
        std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> pending;
        std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> create;
        {
            std::scoped_lock lock(media_dialog.pending_mutex);
            pending = std::move(media_dialog.pending_mount);
            media_dialog.pending_mount.reset();
            create = std::move(media_dialog.pending_create);
            media_dialog.pending_create.reset();
        }
        if (pending) {
            const auto& [drive_index, geometry_index, path] = *pending;
            auto& drive = machine.drive(drive_index);
            if (!machine.media_change_allowed()) {
                media_dialog.message = "Controller is busy; image was not changed";
            } else if (drive.dirty()) {
                media_dialog.message = "Save or eject the modified image first";
            } else {
                const auto* geometry = geometry_index == 0
                    ? nullptr : vz256::floppy_geometries::find(geometry_names[geometry_index]);
                vz256::FloppyImage replacement;
                const bool loaded = geometry != nullptr ? replacement.load(path, *geometry)
                                                        : replacement.load(path);
                if (loaded) {
                    drive = std::move(replacement);
                    media_dialog.message = "Image mounted";
                } else media_dialog.message = "Unknown geometry or invalid image size";
            }
        }
        if (create) {
            const auto& [drive_index, geometry_index, path] = *create;
            auto& drive = machine.drive(drive_index);
            const auto* geometry = geometry_index == 0
                ? nullptr : vz256::floppy_geometries::find(geometry_names[geometry_index]);
            if (!machine.media_change_allowed()) {
                media_dialog.message = "Controller is busy; image was not created";
            } else if (drive.dirty()) {
                media_dialog.message = "Save or eject the modified image first";
            } else if (geometry == nullptr) {
                media_dialog.message = "Select a geometry before creating an image";
            } else {
                vz256::FloppyImage replacement;
                if (replacement.create(path, *geometry, 0xe5)) {
                    drive = std::move(replacement);
                    media_dialog.message = "Blank 0xE5 image created and mounted";
                } else media_dialog.message = "Unable to create image";
            }
        }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_F10) {
                    media_dialog.open = !media_dialog.open;
                    media_dialog.confirm_eject.reset();
                    media_dialog.message.clear();
                } else if (event.key.key == SDLK_F5) {
                    paused = !paused;
                } else if (event.key.key == SDLK_RETURN &&
                           (event.key.mod & SDL_KMOD_ALT) != 0) {
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(window.get(), fullscreen);
                } else if (media_dialog.open) {
                    auto& drive = machine.drive(media_dialog.drive);
                    if (media_dialog.confirm_eject) {
                        if (event.key.key == SDLK_ESCAPE) media_dialog.confirm_eject.reset();
                        else if (event.key.key == SDLK_D) {
                            drive.eject();
                            media_dialog.confirm_eject.reset();
                            media_dialog.message = "Changes discarded; image ejected";
                        } else if (event.key.key == SDLK_S) {
                            if (drive.save()) {
                                drive.eject();
                                media_dialog.confirm_eject.reset();
                                media_dialog.message = "Image saved and ejected";
                            } else media_dialog.message = "Unable to save image";
                        }
                    } else if (event.key.key == SDLK_UP) {
                        media_dialog.drive = (media_dialog.drive + 3) % 4;
                    } else if (event.key.key == SDLK_DOWN) {
                        media_dialog.drive = (media_dialog.drive + 1) % 4;
                    } else if (event.key.key == SDLK_LEFT) {
                        media_dialog.geometry = (media_dialog.geometry +
                                                 geometry_names.size() - 1) % geometry_names.size();
                    } else if (event.key.key == SDLK_RIGHT) {
                        media_dialog.geometry = (media_dialog.geometry + 1) % geometry_names.size();
                    } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_O) {
                        if (!machine.media_change_allowed())
                            media_dialog.message = "Controller is busy";
                        else if (drive.dirty())
                            media_dialog.message = "Save or eject the modified image first";
                        else open_image_picker(window.get(), media_dialog);
                    } else if (event.key.key == SDLK_N) {
                        if (!machine.media_change_allowed())
                            media_dialog.message = "Controller is busy";
                        else if (drive.dirty())
                            media_dialog.message = "Save or eject the modified image first";
                        else if (media_dialog.geometry == 0)
                            media_dialog.message = "Select a geometry before creating an image";
                        else open_new_image_picker(window.get(), media_dialog);
                    } else if (event.key.key == SDLK_E && drive.mounted()) {
                        if (!machine.media_change_allowed())
                            media_dialog.message = "Controller is busy";
                        else if (drive.dirty()) media_dialog.confirm_eject = media_dialog.drive;
                        else {
                            drive.eject();
                            media_dialog.message = "Image ejected";
                        }
                    } else if (event.key.key == SDLK_S && drive.dirty()) {
                        media_dialog.message = drive.save() ? "Image saved" : "Unable to save image";
                    } else if (event.key.key == SDLK_W && drive.mounted()) {
                        const bool protect = !drive.write_protected();
                        media_dialog.message = drive.set_write_protected(protect)
                            ? (protect ? "Write protection enabled" : "Write protection disabled")
                            : "Image file is not writable";
                    }
                } else if (event.key.key == SDLK_F12) { machine.reset(); cpu.reset(); }
                else if (!ImGui::GetIO().WantCaptureKeyboard) {
                    if (const auto key = special_key(event.key)) machine.key(*key);
                }
            }
            if (!media_dialog.open && !ImGui::GetIO().WantCaptureKeyboard &&
                event.type == SDL_EVENT_TEXT_INPUT) {
                for (const auto* text = reinterpret_cast<const unsigned char*>(event.text.text);
                     *text != 0; ++text) {
                    if (*text >= 0x20 && *text <= 0x7e) machine.key(*text);
                }
            }
        }
        const auto now = std::chrono::steady_clock::now();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now - previous).count();
        previous = now;
        cycle_fraction += static_cast<std::uint64_t>(micros) * vz256::Machine::cpu_hz;
        auto cycles = static_cast<std::uint32_t>(cycle_fraction / 1'000'000U);
        cycle_fraction %= 1'000'000U;
        while (!paused && !media_dialog.open && cycles != 0) {
            const auto slice = std::min(cycles, 20'000U);
            const auto spent = cpu.run(slice);
            if (spent == 0) break;
            cycles = spent >= cycles ? 0 : cycles - spent;
        }
        const auto printed = machine.take_printer_output();
        if (printer && !printed.empty()) {
            printer.write(reinterpret_cast<const char*>(printed.data()),
                          static_cast<std::streamsize>(printed.size()));
            printer.flush();
            if (!printer) {
                std::cerr << "Unable to write printer output " << options.printer << '\n';
                running = false;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        const auto actions = draw_main_ui(machine, media_dialog, paused, fullscreen,
                                          ui_state, options.printer);
        draw_media_dialog(window.get(), machine, media_dialog);
        if (actions.reset) { machine.reset(); cpu.reset(); }
        if (actions.quit) running = false;
        if (actions.toggle_pause) paused = !paused;
        if (actions.toggle_fullscreen) {
            fullscreen = !fullscreen;
            SDL_SetWindowFullscreen(window.get(), fullscreen);
        }
        if (actions.select_printer) open_printer_picker(window.get(), printer_dialog);
        if (actions.disconnect_printer) {
            printer.close();
            options.printer.clear();
        }
        ImGui::Render();

        machine.video().render(pixels);
        SDL_UpdateTexture(texture.get(), nullptr, pixels.data(), vz256::Video::width * 4);
        SDL_SetRenderDrawColor(renderer.get(), 16, 16, 16, 255);
        SDL_RenderClear(renderer.get());
        int w{}, h{};
        SDL_GetRenderOutputSize(renderer.get(), &w, &h);
        constexpr float display_width = 800.0F;
        constexpr float display_height = 600.0F;
        constexpr float ui_top = 20.0F;
        constexpr float ui_bottom = 28.0F;
        const float available_height = std::max(
            1.0F, static_cast<float>(h) - ui_top - ui_bottom);
        float output_width = static_cast<float>(w);
        float output_height = available_height;
        if (ui_state.keep_aspect) {
            float scale = std::min(output_width / display_width,
                                   output_height / display_height);
            if (ui_state.integer_scaling && scale >= 1.0F) scale = std::floor(scale);
            output_width = display_width * scale;
            output_height = display_height * scale;
        }
        const SDL_FRect destination{(static_cast<float>(w) - output_width) / 2.0F,
                                    ui_top + (available_height - output_height) / 2.0F,
                                    output_width, output_height};
        SDL_RenderTexture(renderer.get(), texture.get(), nullptr, &destination);
        if (ui_state.scanlines) {
            SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 72);
            const float line_step = std::max(2.0F, destination.h / 300.0F);
            for (float y = destination.y + line_step; y < destination.y + destination.h;
                 y += line_step) {
                SDL_RenderLine(renderer.get(), destination.x, y,
                               destination.x + destination.w, y);
            }
            SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_NONE);
        }
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
        SDL_RenderPresent(renderer.get());
        SDL_Delay(1);
    }
    for (std::size_t i = 0; i < 4; ++i) {
        auto& drive = machine.drive(i);
        if (drive.mounted() && drive.dirty() && !drive.write_protected()) drive.save();
    }
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    texture.reset(); renderer.reset(); window.reset();
    SDL_Quit();
}
