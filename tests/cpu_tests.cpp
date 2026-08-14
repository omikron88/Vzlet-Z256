#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

int main() {
    namespace fs = std::filesystem;
    const auto directory = fs::temp_directory_path() / "vz256-cpu-test";
    fs::create_directories(directory);

    // Disable the monitor, store 0x5a in ordinary RAM and halt. This exercises
    // opcode fetch, instruction-data fetch, memory write and I/O write callbacks.
    const unsigned char program[]{
        0x3e, 0x80,       // ld a,0x80
        0xd3, 0xfc,       // out (0xfc),a
        0x3e, 0x5a,       // ld a,0x5a
        0x32, 0x00, 0x40, // ld (0x4000),a
        0x76              // halt
    };
    {
        std::ofstream rom(directory / "monitor.rom", std::ios::binary);
        rom.write(reinterpret_cast<const char*>(program), sizeof(program));
    }
    { std::ofstream characters(directory / "char.rom", std::ios::binary); characters.put('\0'); }

    vz256::Machine machine;
    assert(machine.load_roms(directory / "monitor.rom", directory / "char.rom"));
    vz256::RedcodeCpu cpu(machine);
    machine.reset();
    cpu.reset();
    assert(cpu.run(64) >= 64);
    assert(machine.read(0x4000) == 0x5a);

    // Exercise the complete WD2797 -> PIO B -> Z80 interrupt path in IM1.
    std::vector<unsigned char> interrupt_rom(0x40, 0);
    const unsigned char interrupt_program[]{
        0x31, 0x00, 0x50, // ld sp,0x5000
        0x3e, 0x88, 0xd3, 0xd7, // PIO vector
        0x3e, 0xb7, 0xd3, 0xd7, // enable PIO interrupt
        0xaf, 0xd3, 0xd1,       // track 0
        0x3e, 0x01, 0xd3, 0xd2, // sector 1
        0x3e, 0x88, 0xd3, 0xd0, // read sector
        0xed, 0x56, 0xfb, 0x76, // IM 1; EI; HALT
        0x76
    };
    std::copy(std::begin(interrupt_program), std::end(interrupt_program), interrupt_rom.begin());
    const unsigned char isr[]{0xdb, 0xd3, 0x32, 0x00, 0x40, 0xfb, 0xed, 0x4d};
    std::copy(std::begin(isr), std::end(isr), interrupt_rom.begin() + 0x38);
    {
        std::ofstream rom(directory / "interrupt.rom", std::ios::binary);
        rom.write(reinterpret_cast<const char*>(interrupt_rom.data()),
                  static_cast<std::streamsize>(interrupt_rom.size()));
    }
    {
        std::vector<unsigned char> disk(80 * 2 * 9 * 512);
        disk[0] = 0xa6;
        std::ofstream image(directory / "disk.img", std::ios::binary);
        image.write(reinterpret_cast<const char*>(disk.data()),
                    static_cast<std::streamsize>(disk.size()));
    }
    vz256::Machine interrupt_machine;
    assert(interrupt_machine.load_roms(directory / "interrupt.rom", directory / "char.rom"));
    assert(interrupt_machine.drive(0).load(directory / "disk.img"));
    vz256::RedcodeCpu interrupt_cpu(interrupt_machine);
    interrupt_machine.reset();
    interrupt_cpu.reset();
    assert(interrupt_cpu.run(256) >= 256);
    assert(interrupt_machine.read(0x4000) == 0xa6);

    fs::remove_all(directory);
}
