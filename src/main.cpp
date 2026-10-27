#include "vz256/cpu.hpp"
#ifdef VZ256_EMBEDDED_ROMS
#include "vz256/embedded_resources.hpp"
#endif
#include "vz256/machine.hpp"
#include "tcp_serial.hpp"
#include "about_window.hpp"
#include "configuration_archive.hpp"
#include "debugger.hpp"
#include "media_window.hpp"
#include "profile_window.hpp"
#include "recent_media.hpp"
#include "serial_window.hpp"
#include "shortcut_window.hpp"
#include "tape_window.hpp"
#include "view_settings.hpp"
#include "window_layout.hpp"

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
    std::filesystem::path tape;
    std::array<std::uint16_t, 2> serial_ports{};
    std::array<std::string, 2> serial_hosts{};
    std::array<std::uint16_t, 2> serial_connect_ports{};
    bool debugger{};
};

struct UiActions {
    bool reset{};
    bool quit{};
    bool toggle_pause{};
    bool toggle_fullscreen{};
    bool select_printer{};
    bool disconnect_printer{};
    bool export_configuration{};
    bool import_configuration{};
    bool open_media{};
    bool open_tape{};
    bool open_serial{};
    bool open_debugger{};
    bool open_about{};
    bool open_shortcuts{};
    bool open_profiles{};
};

struct UiState {
    ViewSettings view;
    std::string notification;
    double notification_until{};
    std::array<double, 4> floppy_led_until{};
    std::array<bool, 4> floppy_led_write{};
};

struct PrinterDialog {
    std::mutex mutex;
    bool picker_active{};
    std::optional<std::filesystem::path> pending_path;
};

struct ConfigurationDialog {
    std::mutex mutex;
    bool picker_active{};
    std::optional<std::filesystem::path> pending_export;
    std::optional<std::filesystem::path> pending_import;
};

const char* activity_name(vz256::Wd2797::Activity activity) {
    switch (activity) {
    case vz256::Wd2797::Activity::read: return "READ";
    case vz256::Wd2797::Activity::write: return "WRITE";
    case vz256::Wd2797::Activity::read_address: return "READ ID";
    case vz256::Wd2797::Activity::read_track: return "READ TRACK";
    case vz256::Wd2797::Activity::write_track: return "FORMAT";
    case vz256::Wd2797::Activity::idle: return "IDLE";
    }
    return "IDLE";
}

bool is_write_activity(vz256::Wd2797::Activity activity) {
    return activity == vz256::Wd2797::Activity::write ||
           activity == vz256::Wd2797::Activity::write_track;
}

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

void SDLCALL selected_configuration_export(void* userdata, const char* const* files, int) {
    auto& dialog = *static_cast<ConfigurationDialog*>(userdata);
    std::scoped_lock lock(dialog.mutex);
    dialog.picker_active = false;
    if (files != nullptr && files[0] != nullptr) dialog.pending_export = files[0];
}

void SDLCALL selected_configuration_import(void* userdata, const char* const* files, int) {
    auto& dialog = *static_cast<ConfigurationDialog*>(userdata);
    std::scoped_lock lock(dialog.mutex);
    dialog.picker_active = false;
    if (files != nullptr && files[0] != nullptr) dialog.pending_import = files[0];
}

void open_configuration_picker(SDL_Window* window, ConfigurationDialog& dialog, bool exporting) {
    static constexpr SDL_DialogFileFilter filters[]{{"Vzlet configuration", "vzconfig"},
                                                     {"All files", "*"}};
    {
        std::scoped_lock lock(dialog.mutex);
        if (dialog.picker_active) return;
        dialog.picker_active = true;
    }
    if (exporting)
        SDL_ShowSaveFileDialog(selected_configuration_export, &dialog, window, filters, 2,
                               "vz256-settings.vzconfig");
    else
        SDL_ShowOpenFileDialog(selected_configuration_import, &dialog, window, filters, 2,
                               nullptr, false);
}

std::filesystem::path configuration_directory() {
    char* path = SDL_GetPrefPath("Vzlet", "Vzlet-Z256");
    if (path == nullptr) return {};
    const std::filesystem::path result(path);
    SDL_free(path);
    return result;
}

std::string take_operation_notification(const std::filesystem::path& directory) {
    const auto path = directory / "last-operation.txt";
    std::ifstream input(path);
    std::string notification;
    std::getline(input, notification);
    std::error_code error;
    std::filesystem::remove(path, error);
    return notification;
}

void store_operation_notification(const std::filesystem::path& directory,
                                  std::string_view notification) {
    std::ofstream output(directory / "last-operation.txt", std::ios::trunc);
    if (output) output << notification << '\n';
}

UiActions draw_main_ui(vz256::Machine& machine, bool paused,
                       bool fullscreen, UiState& state,
                       const std::filesystem::path& printer_path,
                       const ShortcutWindow& shortcuts) {
    UiActions actions;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Disk drives", shortcuts.label(ShortcutAction::media)))
                actions.open_media = true;
            if (ImGui::MenuItem("Cassette", shortcuts.label(ShortcutAction::cassette)))
                actions.open_tape = true;
            if (ImGui::MenuItem("Serial links", shortcuts.label(ShortcutAction::serial)))
                actions.open_serial = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Printer output...")) actions.select_printer = true;
            ImGui::BeginDisabled(printer_path.empty());
            if (ImGui::MenuItem("Disconnect printer")) actions.disconnect_printer = true;
            ImGui::EndDisabled();
            ImGui::Separator();
            if (ImGui::MenuItem("Export configuration...")) actions.export_configuration = true;
            if (ImGui::MenuItem("Import configuration...")) actions.import_configuration = true;
            if (ImGui::MenuItem("Configuration profiles...")) actions.open_profiles = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) actions.quit = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Control")) {
            if (ImGui::MenuItem("Debugger", shortcuts.label(ShortcutAction::debugger)))
                actions.open_debugger = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Reset", shortcuts.label(ShortcutAction::reset))) actions.reset = true;
            if (ImGui::MenuItem(paused ? "Resume" : "Pause",
                                shortcuts.label(ShortcutAction::pause))) actions.toggle_pause = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Keyboard shortcuts...")) actions.open_shortcuts = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Fullscreen", "Alt+Enter", fullscreen)) actions.toggle_fullscreen = true;
            ImGui::Separator();
            ImGui::MenuItem("Keep 4:3 aspect", nullptr, &state.view.keep_aspect);
            ImGui::MenuItem("Integer scaling", nullptr, &state.view.integer_scaling);
            ImGui::MenuItem("Scanlines", nullptr, &state.view.scanlines);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Vzlet Z-256")) actions.open_about = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
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
    const auto fdc = machine.floppy_status();
    const auto now = ImGui::GetTime();
    if (!state.notification.empty() && state.notification_until == 0.0)
        state.notification_until = now + 10.0;
    if (fdc.activity != vz256::Wd2797::Activity::idle) {
        state.floppy_led_until[fdc.drive] = now + 0.15;
        state.floppy_led_write[fdc.drive] = is_write_activity(fdc.activity);
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("FDD");
    for (std::size_t i = 0; i < 4; ++i) {
        ImGui::SameLine(0.0F, 4.0F);
        ImGui::PushID(static_cast<int>(i));
        const auto position = ImGui::GetCursorScreenPos();
        const bool active = now < state.floppy_led_until[i];
        const ImU32 color = !active ? IM_COL32(45, 52, 48, 255)
                            : state.floppy_led_write[i]
                                ? IM_COL32(235, 55, 55, 255)
                                : IM_COL32(45, 220, 90, 255);
        ImGui::InvisibleButton("led", ImVec2(20.0F, 16.0F));
        ImGui::GetWindowDrawList()->AddCircleFilled(
            ImVec2(position.x + 6.0F, position.y + 8.0F), 5.0F, color);
        ImGui::GetWindowDrawList()->AddCircle(
            ImVec2(position.x + 6.0F, position.y + 8.0F), 5.0F,
            IM_COL32(150, 160, 155, 255));
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(position.x + 13.0F, position.y), IM_COL32_WHITE,
            std::string(1, static_cast<char>('A' + i)).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Drive %c: %s", static_cast<char>('A' + i),
                              !active ? "idle" : state.floppy_led_write[i]
                                  ? "writing" : "reading");
        ImGui::PopID();
    }
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
    ImGui::SameLine();
    ImGui::Text("FDC:%c T%u H%u S%u %s%s%s", static_cast<char>('A' + fdc.drive),
                fdc.track, fdc.side, fdc.sector, activity_name(fdc.activity),
                fdc.drq ? " DRQ" : "", fdc.intrq ? " INTRQ" : "");
    if (!state.notification.empty() && now < state.notification_until) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.35F, 0.9F, 0.55F, 1.0F), "%s",
                           state.notification.c_str());
    } else if (state.notification_until != 0.0) {
        state.notification.clear();
    }
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
        else if (option == "--tape" && i + 1 < argc) options.tape = argv[++i];
        else if (option == "--debugger") options.debugger = true;
        else if ((option == "--serial-a" || option == "--serial-b") && i + 1 < argc) {
            try {
                const auto value = std::stoul(argv[++i]);
                if (value == 0 || value > 65535) return false;
                options.serial_ports[option.back() == 'a' ? 0 : 1] =
                    static_cast<std::uint16_t>(value);
            } catch (...) { return false; }
        } else if ((option == "--serial-a-connect" || option == "--serial-b-connect") &&
                   i + 1 < argc) {
            const std::string endpoint = argv[++i];
            const auto separator = endpoint.rfind(':');
            if (separator == std::string::npos || separator == 0) return false;
            try {
                const auto value = std::stoul(endpoint.substr(separator + 1));
                if (value == 0 || value > 65535) return false;
                const auto channel = option[9] == 'a' ? 0U : 1U;
                options.serial_hosts[channel] = endpoint.substr(0, separator);
                options.serial_connect_ports[channel] = static_cast<std::uint16_t>(value);
            } catch (...) { return false; }
        }
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
    for (std::size_t channel = 0; channel < 2; ++channel)
        if (options.serial_ports[channel] != 0 && !options.serial_hosts[channel].empty())
            return false;
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
                     "[--geometry-a PROFILE] [--read-only-a] [--printer FILE] "
                     "[--tape FILE.wav] [--serial-a PORT] [--serial-b PORT] "
                     "[--serial-a-connect HOST:PORT] [--serial-b-connect HOST:PORT] "
                     "[--debugger] (a..d)\n";
        return 2;
    }

    vz256::Machine machine;
    SerialLinks serial{
        std::make_unique<TcpSerial>(options.serial_ports[0], options.serial_hosts[0],
                                    options.serial_connect_ports[0]),
        std::make_unique<TcpSerial>(options.serial_ports[1], options.serial_hosts[1],
                                    options.serial_connect_ports[1])};
    for (std::size_t channel = 0; channel < serial.size(); ++channel) {
        if ((options.serial_ports[channel] != 0 || !options.serial_hosts[channel].empty()) &&
            !serial[channel]->valid()) {
            std::cerr << "Unable to initialize serial channel " << char('A' + channel) << '\n';
            return 1;
        }
    }
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
    if (!options.tape.empty()) {
        const bool exists = std::filesystem::exists(options.tape);
        const bool ready = exists ? machine.cassette().load_wav(options.tape)
                                  : machine.cassette().create_wav(options.tape);
        if (!ready) {
            std::cerr << "Unable to open or create PCM WAV cassette " << options.tape << '\n';
            return 1;
        }
        machine.cassette().set_motor(exists);
    }

    std::ofstream printer;
    if (!options.printer.empty()) {
        printer.open(options.printer, std::ios::binary | std::ios::app);
        if (!printer) {
            std::cerr << "Unable to open printer output " << options.printer << '\n';
            return 1;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
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
    std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> tape_audio(
        nullptr, SDL_DestroyAudioStream);
    std::uint32_t tape_audio_rate = 0;
    if (!window || !renderer || !texture) {
        std::cerr << "SDL window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }
    restore_window_layout(window.get(), "main");
    UiState ui_state;
    ui_state.view = load_view_settings();
    ui_state.notification = take_operation_notification(configuration_directory());
    bool fullscreen = ui_state.view.fullscreen;
    if (fullscreen) SDL_SetWindowFullscreen(window.get(), true);
    SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
    SDL_StartTextInput(window.get());
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiContext* main_imgui_context = ImGui::GetCurrentContext();
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
    auto about_window = std::make_unique<AboutWindow>(window.get());
    auto debugger = std::make_unique<DebuggerWindow>(window.get());
    RecentMedia recent_media(configuration_directory() / "recent-media.cfg");
    auto media_window = std::make_unique<MediaWindow>(window.get(), machine, recent_media);
    auto profile_window = std::make_unique<ProfileWindow>(window.get(), configuration_directory());
    auto serial_window = std::make_unique<SerialWindow>(window.get(), serial);
    auto shortcut_window = std::make_unique<ShortcutWindow>(window.get());
    auto tape_window = std::make_unique<TapeWindow>(window.get(), machine, options.tape,
                                                    recent_media);
    machine.reset();
    cpu.reset();
    std::vector<std::uint32_t> pixels(vz256::Video::width * vz256::Video::height);
    PrinterDialog printer_dialog;
    ConfigurationDialog configuration_dialog;
    std::optional<std::filesystem::path> configuration_export;
    std::optional<std::filesystem::path> configuration_import;
    for (std::size_t channel = 0; channel < serial.size(); ++channel) {
        if (options.serial_ports[channel] != 0) {
            serial_window->configure(channel, TcpSerial::Mode::listen, {},
                                     options.serial_ports[channel]);
        } else if (!options.serial_hosts[channel].empty()) {
            serial_window->configure(channel, TcpSerial::Mode::connect,
                                     options.serial_hosts[channel],
                                     options.serial_connect_ports[channel]);
        }
    }
    bool running = true;
    bool paused = false;
    auto previous = std::chrono::steady_clock::now();
    std::uint64_t cycle_fraction = 0;
    if (options.debugger) debugger->open(paused);
    while (running) {
        tape_window->update();
        if (const auto request = profile_window->take_request()) {
            if (request->operation == ProfileOperation::save)
                configuration_export = request->archive;
            else
                configuration_import = request->archive;
            running = false;
        }
        {
            std::scoped_lock lock(configuration_dialog.mutex);
            if (configuration_dialog.pending_export) {
                configuration_export = std::move(configuration_dialog.pending_export);
                configuration_dialog.pending_export.reset();
                running = false;
            } else if (configuration_dialog.pending_import) {
                configuration_import = std::move(configuration_dialog.pending_import);
                configuration_dialog.pending_import.reset();
                running = false;
            }
        }
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
        media_window->update();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (about_window->handle_event(event)) {
                ImGui::SetCurrentContext(main_imgui_context);
                continue;
            }
            if (debugger->handle_event(event, paused)) {
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F3 && paused)
                    cpu.step();
                ImGui::SetCurrentContext(main_imgui_context);
                continue;
            }
            if (media_window->handle_event(event)) {
                ImGui::SetCurrentContext(main_imgui_context);
                continue;
            }
            if (profile_window->handle_event(event)) {
                ImGui::SetCurrentContext(main_imgui_context);
                continue;
            }
            if (serial_window->handle_event(event)) {
                ImGui::SetCurrentContext(main_imgui_context);
                continue;
            }
            if (shortcut_window->handle_event(event)) {
                ImGui::SetCurrentContext(main_imgui_context);
                continue;
            }
            if (tape_window->handle_event(event)) {
                ImGui::SetCurrentContext(main_imgui_context);
                continue;
            }
            ImGui::SetCurrentContext(main_imgui_context);
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == shortcut_window->key(ShortcutAction::media)) {
                    media_window->open();
                } else if (event.key.key == shortcut_window->key(ShortcutAction::cassette)) {
                    tape_window->open();
                } else if (event.key.key == shortcut_window->key(ShortcutAction::serial)) {
                    serial_window->open();
                } else if (event.key.key == shortcut_window->key(ShortcutAction::debugger)) {
                    debugger->open(paused);
                } else if (event.key.key == shortcut_window->key(ShortcutAction::pause)) {
                    paused = !paused;
                } else if (event.key.key == SDLK_RETURN &&
                           (event.key.mod & SDL_KMOD_ALT) != 0) {
                    if (!fullscreen) save_window_layout(window.get(), "main");
                    fullscreen = !fullscreen;
                    ui_state.view.fullscreen = fullscreen;
                    save_view_settings(ui_state.view);
                    SDL_SetWindowFullscreen(window.get(), fullscreen);
                } else if (event.key.key == shortcut_window->key(ShortcutAction::reset)) {
                    machine.reset(); cpu.reset();
                }
                else if (!ImGui::GetIO().WantCaptureKeyboard) {
                    if (const auto key = special_key(event.key)) machine.key(*key);
                }
            }
            if (!ImGui::GetIO().WantCaptureKeyboard &&
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
        while (!paused && !media_window->active() && cycles != 0) {
            if (debugger->active() && debugger->breakpoint_hit(cpu.program_counter())) {
                paused = true;
                break;
            }
            const auto slice = std::min(cycles, 20'000U);
            const auto spent = debugger->active() ? cpu.step() : cpu.run(slice);
            if (spent == 0) break;
            cycles = spent >= cycles ? 0 : cycles - spent;
        }
        for (std::uint8_t channel = 0; channel < serial.size(); ++channel) {
            for (const auto byte : serial[channel]->receive())
                machine.serial_receive(channel, byte);
            serial[channel]->send(machine.take_serial_output(channel));
        }
        auto monitor_samples = machine.cassette().take_monitor_samples();
        if (!machine.cassette().monitor_enabled()) {
            tape_audio.reset();
            tape_audio_rate = 0;
        } else if (!monitor_samples.empty()) {
            const auto rate = static_cast<std::uint32_t>(
                machine.cassette().sample_rate() * machine.cassette().playback_speed());
            if (!tape_audio || tape_audio_rate != rate) {
                SDL_AudioSpec spec{};
                spec.format = SDL_AUDIO_S16;
                spec.channels = 2;
                spec.freq = static_cast<int>(rate);
                tape_audio.reset(SDL_OpenAudioDeviceStream(
                    SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr));
                tape_audio_rate = tape_audio ? rate : 0;
                if (tape_audio) SDL_ResumeAudioStreamDevice(tape_audio.get());
            }
            if (tape_audio && !SDL_PutAudioStreamData(
                    tape_audio.get(), monitor_samples.data(),
                    static_cast<int>(monitor_samples.size() * sizeof(std::int16_t)))) {
                std::cerr << "Unable to play cassette monitor: " << SDL_GetError() << '\n';
                machine.cassette().set_monitor_enabled(false);
                tape_audio.reset();
                tape_audio_rate = 0;
            }
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
        const auto previous_view = ui_state.view;
        const auto actions = draw_main_ui(machine, paused, fullscreen, ui_state, options.printer,
                                          *shortcut_window);
        if (actions.reset) { machine.reset(); cpu.reset(); }
        if (actions.quit) running = false;
        if (actions.toggle_pause) paused = !paused;
        if (actions.toggle_fullscreen) {
            if (!fullscreen) save_window_layout(window.get(), "main");
            fullscreen = !fullscreen;
            ui_state.view.fullscreen = fullscreen;
            SDL_SetWindowFullscreen(window.get(), fullscreen);
        }
        if (ui_state.view != previous_view) save_view_settings(ui_state.view);
        if (actions.select_printer) open_printer_picker(window.get(), printer_dialog);
        if (actions.disconnect_printer) {
            printer.close();
            options.printer.clear();
        }
        if (actions.export_configuration)
            open_configuration_picker(window.get(), configuration_dialog, true);
        if (actions.import_configuration)
            open_configuration_picker(window.get(), configuration_dialog, false);
        if (actions.open_debugger) debugger->open(paused);
        if (actions.open_about) about_window->open();
        if (actions.open_media) media_window->open();
        if (actions.open_profiles) profile_window->open();
        if (actions.open_serial) serial_window->open();
        if (actions.open_tape) tape_window->open();
        if (actions.open_shortcuts) shortcut_window->open();
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
        if (ui_state.view.keep_aspect) {
            float scale = std::min(output_width / display_width,
                                   output_height / display_height);
            if (ui_state.view.integer_scaling && scale >= 1.0F) scale = std::floor(scale);
            output_width = display_width * scale;
            output_height = display_height * scale;
        }
        const SDL_FRect destination{(static_cast<float>(w) - output_width) / 2.0F,
                                    ui_top + (available_height - output_height) / 2.0F,
                                    output_width, output_height};
        SDL_RenderTexture(renderer.get(), texture.get(), nullptr, &destination);
        if (ui_state.view.scanlines) {
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
        debugger->render(machine, cpu, paused);
        ImGui::SetCurrentContext(main_imgui_context);
        about_window->render();
        ImGui::SetCurrentContext(main_imgui_context);
        media_window->render();
        ImGui::SetCurrentContext(main_imgui_context);
        profile_window->render();
        ImGui::SetCurrentContext(main_imgui_context);
        serial_window->render();
        ImGui::SetCurrentContext(main_imgui_context);
        shortcut_window->render();
        ImGui::SetCurrentContext(main_imgui_context);
        tape_window->render();
        ImGui::SetCurrentContext(main_imgui_context);
        SDL_Delay(1);
    }
    for (std::size_t i = 0; i < 4; ++i) {
        auto& drive = machine.drive(i);
        if (drive.mounted() && drive.dirty() && !drive.write_protected()) drive.save();
    }
    if (machine.cassette().recording() && !machine.cassette().save_wav())
        std::cerr << "Unable to save cassette WAV " << options.tape << '\n';
    save_view_settings(ui_state.view);
    if (!fullscreen) save_window_layout(window.get(), "main");
    about_window.reset();
    media_window.reset();
    profile_window.reset();
    tape_window.reset();
    serial_window.reset();
    shortcut_window.reset();
    debugger.reset();
    std::string configuration_error;
    const auto preferences = configuration_directory();
    if (configuration_export) {
        const bool succeeded = export_configuration(preferences, *configuration_export,
                                                     configuration_error);
        if (!succeeded) std::cerr << "Unable to export configuration: " << configuration_error << '\n';
        store_operation_notification(preferences, succeeded
            ? "Configuration exported successfully"
            : "Configuration export failed: " + configuration_error);
    }
    if (configuration_import) {
        const bool succeeded = import_configuration(preferences, *configuration_import,
                                                     configuration_error);
        if (!succeeded) std::cerr << "Unable to import configuration: " << configuration_error << '\n';
        store_operation_notification(preferences, succeeded
            ? "Configuration imported successfully"
            : "Configuration import failed: " + configuration_error);
    }
    ImGui::SetCurrentContext(main_imgui_context);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    tape_audio.reset(); texture.reset(); renderer.reset(); window.reset();
    SDL_Quit();
}
