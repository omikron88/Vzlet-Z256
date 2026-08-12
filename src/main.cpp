#include "vz256/machine.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <vector>

static std::uint8_t translate(SDL_Keycode k, SDL_Keymod m) {
  if (k >= SDLK_A && k <= SDLK_Z) return static_cast<std::uint8_t>((m & SDL_KMOD_CTRL) ? k-SDLK_A+1 : k);
  if (k >= 0x20 && k < 0x7f) return static_cast<std::uint8_t>(k);
  switch(k) { case SDLK_RETURN:return 13; case SDLK_TAB:return 9; case SDLK_ESCAPE:return 27;
    case SDLK_BACKSPACE:return 0x7f; case SDLK_DELETE:return (m&SDL_KMOD_SHIFT)?0x81:0x80;
    case SDLK_INSERT:return (m&SDL_KMOD_SHIFT)?0x83:0x82; case SDLK_END:return (m&SDL_KMOD_SHIFT)?0x85:0x84;
    case SDLK_HOME:return 0x8d; case SDLK_UP:return 0xc1; case SDLK_DOWN:return 0xc2;
    case SDLK_RIGHT:return 0xc3; case SDLK_LEFT:return 0xc4; case SDLK_F1:return 0xd0;
    case SDLK_F2:return 0xd1; case SDLK_F3:return 0xd2; default:return 0; }
}
int main(int argc, char **argv) {
  std::filesystem::path root=VZ256_DEFAULT_DATA_DIR;
  std::filesystem::path rom=argc>1?argv[1]:root/"roms/boot.rom", disk=argc>2?argv[2]:root/"disks/boot.img";
  vz256::Machine machine;
  if (!machine.load_rom(rom) || !machine.mount_disk(disk)) { std::cerr<<"Nelze nacist ROM nebo disk\n"; return 1; }
  if (!SDL_Init(SDL_INIT_VIDEO)) { std::cerr<<SDL_GetError()<<'\n'; return 1; }
  SDL_Window *win=nullptr; SDL_Renderer *renderer=nullptr;
  if (!SDL_CreateWindowAndRenderer("Vzlet Z-256",960,720,SDL_WINDOW_RESIZABLE,&win,&renderer)) return 1;
  auto *texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,vz256::Machine::width,vz256::Machine::height);
  SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST); std::vector<std::uint32_t> frame(vz256::Machine::width*vz256::Machine::height);
  bool running=true; std::uint64_t previous=SDL_GetTicksNS();
  while(running) { SDL_Event e; while(SDL_PollEvent(&e)) { if(e.type==SDL_EVENT_QUIT) running=false;
      if(e.type==SDL_EVENT_KEY_DOWN) { if(e.key.key==SDLK_F12) machine.reset(); else if(auto c=translate(e.key.key,e.key.mod)) machine.key(c); } }
    const auto now=SDL_GetTicksNS(); machine.run_cycles(static_cast<std::uint32_t>((now-previous)*4'000'000/1'000'000'000)); previous=now;
    machine.render(frame); SDL_UpdateTexture(texture,nullptr,frame.data(),vz256::Machine::width*4);
    SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderClear(renderer); SDL_RenderTexture(renderer,texture,nullptr,nullptr); SDL_RenderPresent(renderer); SDL_Delay(1);
  }
  SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(win); SDL_Quit();
}
