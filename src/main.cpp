#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace {

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
    std::filesystem::path root = ".";
    if (argc == 3 && std::string_view(argv[1]) == "--resources") root = argv[2];

    vz256::Machine machine;
    if (!machine.load_roms(root / "roms/boot.rom", root / "roms/char.rom")) {
        std::cerr << "Unable to load boot and character ROMs from " << root << '\n';
        return 1;
    }
    if (std::filesystem::exists(root / "disks/boot.img")) machine.drive(0).load(root / "disks/boot.img");

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
    bool running = true;
    auto previous = std::chrono::steady_clock::now();
    std::uint64_t cycle_fraction = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_F12) { machine.reset(); cpu.reset(); }
                else if (const auto key = special_key(event.key)) machine.key(*key);
            }
            if (event.type == SDL_EVENT_TEXT_INPUT) {
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
        while (cycles != 0) {
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
        SDL_RenderPresent(renderer.get());
        SDL_Delay(1);
    }
    machine.drive(0).save();
    texture.reset(); renderer.reset(); window.reset();
    SDL_Quit();
}
