#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>

int main() {
    namespace fs = std::filesystem;
    const auto directory = fs::temp_directory_path() / "vz256-cpu-test";
    fs::create_directories(directory);

    // Store 0x5a in ordinary RAM and halt. This exercises opcode fetch,
    // instruction-data fetch and the memory-write callback.
    const unsigned char program[]{
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

    fs::remove_all(directory);
}
