#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

#include <cassert>
#include <filesystem>
#include <vector>

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::filesystem::path root = argv[1];
    vz256::Machine machine;
    assert(machine.load_roms(root / "roms/boot.rom", root / "roms/char.rom"));
    assert(machine.drive(0).load(root / "disks/boot.img"));
    vz256::RedcodeCpu cpu(machine);
    machine.reset();
    cpu.reset();

    // This covers the monitor's PIO/CTC setup, index detection, WD2797
    // multi-sector transfer in IM2 and execution of the disk loader.
    cpu.run(20'000'000);
    // CP/M has printed its prompt and is polling BIOS CONIN at 0xFAE1..0xFAE6.
    assert(cpu.program_counter() >= 0xFAE1 && cpu.program_counter() <= 0xFAE6);
    machine.output(0xFC, 0x80); // inspect RAM page zero
    // The loader at 0xF000 is subsequently overwritten by the 0xE400-0xF9FF
    // CP/M image. Verify the final CCP/BDOS signature instead.
    assert(machine.read(0xE400) == 0xC3);
    assert(machine.read(0xE401) == 0x5C);
    std::vector<std::uint32_t> before(vz256::Video::width * vz256::Video::height);
    std::vector<std::uint32_t> after(before.size());
    machine.video().render(before);

    // Type a real CP/M command through the active-low PIO-A keyboard path.
    for (const auto key : {'D', 'I', 'R', '\r'}) machine.key(static_cast<std::uint8_t>(key));
    cpu.run(20'000'000);
    assert(cpu.program_counter() >= 0xFAE1 && cpu.program_counter() <= 0xFAE6);
    assert(!machine.interrupt_pending());
    machine.video().render(after);
    assert(before != after);
}
