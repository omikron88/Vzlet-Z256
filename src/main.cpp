#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
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
};

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

void draw_media_dialog(SDL_Renderer* renderer, vz256::Machine& machine,
                       const MediaDialog& dialog) {
    SDL_FRect panel{90.0F, 85.0F, 780.0F, 550.0F};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 12, 20, 245);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 100, 180, 255, 255);
    SDL_RenderRect(renderer, &panel);
    SDL_SetRenderScale(renderer, 2.0F, 2.0F);
    SDL_SetRenderDrawColor(renderer, 230, 240, 255, 255);
    SDL_RenderDebugText(renderer, 58.0F, 52.0F, "DISK DRIVES (F10 closes)");
    for (std::size_t i = 0; i < 4; ++i) {
        const auto& drive = machine.drive(i);
        std::string line = i == dialog.drive ? "> " : "  ";
        line += static_cast<char>('A' + i);
        line += ": ";
        if (!drive.mounted()) line += "<empty>";
        else {
            line += drive.path().filename().string();
            line += "  [" + drive.geometry().name + "]";
            if (drive.write_protected()) line += " RO";
            if (drive.dirty()) line += " *";
        }
        SDL_RenderDebugText(renderer, 58.0F, 76.0F + static_cast<float>(i) * 18.0F,
                            line.c_str());
    }
    const std::string geometry = "Next image geometry: " +
                                 std::string(geometry_names[dialog.geometry]);
    SDL_RenderDebugText(renderer, 58.0F, 158.0F, geometry.c_str());
    SDL_RenderDebugText(renderer, 58.0F, 178.0F,
                        "UP/DOWN drive  LEFT/RIGHT geometry");
    SDL_RenderDebugText(renderer, 58.0F, 190.0F,
                        "ENTER/O mount  E eject  S save  W write protect");
    if (dialog.confirm_eject) {
        SDL_SetRenderDrawColor(renderer, 255, 220, 100, 255);
        SDL_RenderDebugText(renderer, 58.0F, 210.0F,
                            "Modified image: S save+eject, D discard, ESC cancel");
    } else if (!dialog.message.empty()) {
        SDL_SetRenderDrawColor(renderer, 255, 220, 100, 255);
        SDL_RenderDebugText(renderer, 58.0F, 210.0F, dialog.message.c_str());
    }
    SDL_SetRenderScale(renderer, 1.0F, 1.0F);
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
                     "[--geometry-a PROFILE] [--read-only-a] (a..d)\n";
        return 2;
    }

    vz256::Machine machine;
    if (!machine.load_roms(options.resources / "roms/boot.rom",
                           options.resources / "roms/char.rom")) {
        std::cerr << "Unable to load boot and character ROMs from " << options.resources << '\n';
        return 1;
    }
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

    vz256::RedcodeCpu cpu(machine);
    machine.reset();
    cpu.reset();
    std::vector<std::uint32_t> pixels(vz256::Video::width * vz256::Video::height);
    MediaDialog media_dialog;
    bool running = true;
    auto previous = std::chrono::steady_clock::now();
    std::uint64_t cycle_fraction = 0;
    while (running) {
        std::optional<std::tuple<std::size_t, std::size_t, std::filesystem::path>> pending;
        {
            std::scoped_lock lock(media_dialog.pending_mutex);
            pending = std::move(media_dialog.pending_mount);
            media_dialog.pending_mount.reset();
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
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_F10) {
                    media_dialog.open = !media_dialog.open;
                    media_dialog.confirm_eject.reset();
                    media_dialog.message.clear();
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
                else if (const auto key = special_key(event.key)) machine.key(*key);
            }
            if (!media_dialog.open && event.type == SDL_EVENT_TEXT_INPUT) {
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
        while (!media_dialog.open && cycles != 0) {
            const auto slice = std::min(cycles, 20'000U);
            const auto spent = cpu.run(slice);
            if (spent == 0) break;
            cycles = spent >= cycles ? 0 : cycles - spent;
        }

        machine.video().render(pixels);
        SDL_UpdateTexture(texture.get(), nullptr, pixels.data(), vz256::Video::width * 4);
        SDL_SetRenderDrawColor(renderer.get(), 16, 16, 16, 255);
        SDL_RenderClear(renderer.get());
        int w{}, h{};
        SDL_GetRenderOutputSize(renderer.get(), &w, &h);
        const float scale = std::min(w / 800.0F, h / 600.0F);
        const SDL_FRect destination{(w - 800 * scale) / 2, (h - 600 * scale) / 2,
                                    800 * scale, 600 * scale};
        SDL_RenderTexture(renderer.get(), texture.get(), nullptr, &destination);
        if (media_dialog.open) draw_media_dialog(renderer.get(), machine, media_dialog);
        SDL_RenderPresent(renderer.get());
        SDL_Delay(1);
    }
    for (std::size_t i = 0; i < 4; ++i) {
        auto& drive = machine.drive(i);
        if (drive.mounted() && drive.dirty() && !drive.write_protected()) drive.save();
    }
    texture.reset(); renderer.reset(); window.reset();
    SDL_Quit();
}
