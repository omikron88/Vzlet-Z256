#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

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
                else if (event.key.key >= 0x20 && event.key.key <= 0x7e)
                    machine.key(static_cast<std::uint8_t>(event.key.key));
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
