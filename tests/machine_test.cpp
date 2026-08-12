#include "vz256/machine.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <vector>
int main() {
  vz256::Machine m; assert(m.load_rom(SOURCE_DIR "/roms/boot.rom"));
  assert(m.read(0)==0xc3); m.write(0x4000,0x5a); assert(m.read(0x4000)==0x5a);
  m.out(0xfc,0x85); assert(m.bank()==0x85); m.write(0x1234,0x77); assert(m.read(0x1234)==0x77);
  m.key('A'); assert((m.in(0xf1)&1)==0); assert(m.in(0xf0)=='A'); assert(m.in(0xf1)==0xff);
  assert(m.mount_disk(SOURCE_DIR "/disks/boot.img")); m.out(0xd1,0); m.out(0xd2,1); m.out(0xd0,0x88); assert(m.in(0xd0)&1); (void)m.in(0xd3);
  std::vector<std::uint32_t> pixels(vz256::Machine::width*vz256::Machine::height); m.render(pixels); assert(pixels[0]==0xff101410);
  m.run_cycles(4000); assert(m.cycles()==4000); std::cout<<"all tests passed\n";
}
