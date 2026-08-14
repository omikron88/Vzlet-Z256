#include "vz256/machine.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

int main() {
    namespace fs = std::filesystem;
    const auto temp = fs::temp_directory_path() / "vz256-core-test";
    fs::create_directories(temp);
    { std::ofstream f(temp / "monitor.rom", std::ios::binary); f.put('\x42'); f.put('\x24'); }
    { std::ofstream f(temp / "char.rom", std::ios::binary); f.put('\x5a'); }

    vz256::Machine machine;
    assert(machine.load_roms(temp / "monitor.rom", temp / "char.rom"));
    machine.reset();
    assert(machine.read(0) == 0x42);
    assert(machine.read(2) == 0x42); // short ROM is mirrored
    machine.write(0, 0xff);
    assert(machine.read(0) == 0x42); // EPROM stays read-only
    machine.write(0x2000, 0xa5);
    assert(machine.read(0x2000) == 0xa5);

    machine.output(0xfc, 0x80 | 0x00 | (1 << 2) | (2 << 4));
    machine.write(0x1234, 0x71); // write page 1
    assert(machine.read(0x1234) == 0); // read page 0
    assert(machine.read(0x1234, true) == 0); // opcode page 2
    machine.output(0xfc, 0x80 | 0x01 | (1 << 2) | (2 << 4));
    assert(machine.read(0x1234) == 0x71);

    machine.output(0xc1, 0); // video bus
    machine.video().write(3, 0, 1);  // R1: displayed columns
    machine.video().write(3, 1, 80);
    machine.video().write(3, 0, 6);  // R6: displayed character rows
    machine.video().write(3, 1, 24);
    machine.video().write(3, 0, 9);  // R9: scanlines per character - 1
    machine.video().write(3, 1, 11);
    machine.output(0xfc, 0x80 | 0x01 | (1 << 2));
    machine.write(0x8000, 0x80);
    machine.write(0x8100, 0x40); // next raster line is 0x100 bytes away
    std::vector<std::uint32_t> pixels(vz256::Video::width * vz256::Video::height);
    machine.video().render(pixels);
    assert(pixels[0] == 0xffaaaaaaU);
    assert(pixels[1] == 0xff000000U);
    assert(pixels[vz256::Video::width] == 0xff000000U);
    assert(pixels[vz256::Video::width + 1] == 0xffaaaaaaU);

    // CRTC start address 0x100 is wired to VRAM address 0x9000, not 0x8100.
    machine.video().write(3, 0, 12);
    machine.video().write(3, 1, 1);
    machine.video().write(3, 0, 13);
    machine.video().write(3, 1, 0);
    machine.write(0x9000, 0x20);
    machine.video().render(pixels);
    assert(pixels[2] == 0xffaaaaaaU);

    machine.key('A');
    assert(machine.input(0xd6) == 0x80);
    assert(machine.input(0xd4) == 'A');
    assert(machine.input(0xd6) == 0x00);
    fs::remove_all(temp);
}
