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

    machine.output(0xd7, 0x88); // PIO base vector: channel A uses 0x8a
    machine.output(0xd6, 0x83); // enable keyboard interrupt
    machine.key('A');
    machine.key(0x0d);
    assert(machine.interrupt_pending());
    assert(machine.interrupt_vector() == 0x8a);
    assert(machine.input(0xd4) == static_cast<std::uint8_t>(~'A'));
    assert(machine.interrupt_pending());
    assert(machine.input(0xd4) == static_cast<std::uint8_t>(~0x0d));
    assert(!machine.interrupt_pending());

    // WD2797 read-sector flow and its DRQ signal through PIO channel B.
    {
        std::vector<std::uint8_t> disk(80 * vz256::FloppyImage::sides *
                                      vz256::FloppyImage::sectors_per_track *
                                      vz256::FloppyImage::sector_size);
        disk[0] = 0xde;
        disk[1] = 0xad;
        std::ofstream image(temp / "disk.img", std::ios::binary);
        image.write(reinterpret_cast<const char*>(disk.data()),
                    static_cast<std::streamsize>(disk.size()));
    }
    assert(machine.drive(0).load(temp / "disk.img"));
    machine.output(0xd7, 0x88); // PIO B interrupt vector
    machine.output(0xd7, 0xcf); // mode 3
    machine.output(0xd7, 0xc0); // direction mask, not a vector
    machine.output(0xd7, 0xb7); // enable interrupt on high level, mask follows
    machine.output(0xd7, 0x7f); // monitor DRQ on bit 7
    machine.output(0xd1, 0);    // track 0
    machine.output(0xd2, 1);    // sector 1
    machine.output(0xd0, 0x88); // read sector
    assert((machine.input(0xd5) & 0x80) != 0);
    assert(machine.interrupt_pending());
    assert(machine.interrupt_vector() == 0x88);
    assert(machine.input(0xd3) == 0xde);
    assert(!machine.media_change_allowed());
    assert(!machine.interrupt_pending());
    machine.tick(64);
    assert(machine.input(0xd3) == 0xad);
    for (std::size_t i = 2; i < vz256::FloppyImage::sector_size; ++i) {
        machine.tick(64);
        (void)machine.input(0xd3);
    }
    assert(!machine.interrupt_pending());
    assert((machine.input(0xd0) & vz256::Wd2797::busy) == 0);
    assert(machine.media_change_allowed());

    // A 77-track 8-inch FM image uses 26 128-byte sectors instead of the
    // default 5.25-inch 9x512 layout.
    const auto eight_inch = vz256::floppy_geometries::eight_sssd_77;
    {
        std::vector<std::uint8_t> disk(eight_inch.image_size());
        disk.back() = 0x6c;
        std::ofstream image(temp / "eight.img", std::ios::binary);
        image.write(reinterpret_cast<const char*>(disk.data()),
                    static_cast<std::streamsize>(disk.size()));
    }
    assert(vz256::floppy_geometries::find("8-sssd-77") != nullptr);
    assert(vz256::floppy_geometries::detect(eight_inch.image_size()) != nullptr);
    assert(machine.drive(0).load(temp / "eight.img", eight_inch));
    assert(machine.drive(0).geometry().sector_size == 128);
    assert(machine.drive(0).sector(76, 0, 26).back() == 0x6c);
    assert(machine.drive(0).sector(77, 0, 1).empty());
    assert(machine.drive(0).sector(0, 1, 1).empty());

    machine.output(0xd1, 76);
    machine.output(0xd2, 26);
    machine.output(0xd0, 0xc0); // Read Address
    assert(machine.input(0xd3) == 76);
    machine.tick(128);
    assert(machine.input(0xd3) == 0); // side
    machine.tick(128);
    assert(machine.input(0xd3) == 26);
    machine.tick(128);
    assert(machine.input(0xd3) == 0); // WD2797 N=0 means 128 bytes

    std::array<std::uint8_t, 128> replacement{};
    replacement[0] = 0xa9;
    assert(machine.drive(0).write_sector(0, 0, 1, replacement));
    assert(machine.drive(0).dirty());
    assert(machine.drive(0).save());
    assert(!machine.drive(0).dirty());
    assert(!fs::exists((temp / "eight.img").string() + ".vz256.tmp"));
    assert(machine.drive(0).set_write_protected(true));
    assert(machine.drive(0).write_protected());
    assert(machine.drive(0).set_write_protected(false));
    assert(!machine.drive(0).write_protected());
    machine.drive(0).eject();
    assert(!machine.drive(0).mounted());
    assert(machine.drive(0).load(temp / "eight.img", true)); // auto-detect, read-only
    assert(machine.drive(0).write_protected());
    assert(!machine.drive(0).write_sector(0, 0, 1, replacement));

    // The double-sided 8-inch layout has the IBM 3740 77x26x128 organization
    // on both sides. Exercise auto-detection and side selection through the
    // WD2797, not just direct FloppyImage access.
    const auto double_sided = vz256::floppy_geometries::eight_dssd_77;
    {
        std::vector<std::uint8_t> disk(double_sided.image_size());
        const auto side_one_offset = double_sided.sectors_per_track *
                                     double_sided.sector_size;
        disk[side_one_offset] = 0x5d;
        std::ofstream image(temp / "eight-double-sided.img", std::ios::binary);
        image.write(reinterpret_cast<const char*>(disk.data()),
                    static_cast<std::streamsize>(disk.size()));
    }
    assert(double_sided.image_size() == 512'512);
    const auto* detected = vz256::floppy_geometries::detect(512'512);
    assert(detected != nullptr);
    assert(detected->name == "8-dssd-77");
    assert(machine.drive(0).load(temp / "eight-double-sided.img"));
    assert(machine.drive(0).sector(0, 1, 1).front() == 0x5d);
    machine.output(0xd1, 0);    // track 0
    machine.output(0xd2, 1);    // sector 1
    machine.output(0xd0, 0x82); // Read Sector, side compare/side 1
    assert(machine.input(0xd3) == 0x5d);
    machine.output(0xd0, 0xa2); // Write Sector, side compare/side 1
    for (std::size_t i = 0; i < double_sided.sector_size; ++i) {
        if (i != 0) machine.tick(128);
        machine.output(0xd3, static_cast<std::uint8_t>(i));
    }
    assert(machine.drive(0).sector(0, 1, 1).front() == 0x00);
    assert(machine.drive(0).sector(0, 1, 1).back() == 0x7f);
    assert(machine.drive(0).dirty());

    // Double-density 8-inch media retain 26 sectors per track, but use
    // 256-byte sectors. Verify its unique size and the WD2797 N=1 ID field.
    const auto double_density = vz256::floppy_geometries::eight_dsdd_77;
    {
        std::vector<std::uint8_t> disk(double_density.image_size());
        std::ofstream image(temp / "eight-double-density.img", std::ios::binary);
        image.write(reinterpret_cast<const char*>(disk.data()),
                    static_cast<std::streamsize>(disk.size()));
    }
    assert(double_density.image_size() == 1'025'024);
    assert(double_density.sectors_per_track == 26);
    assert(double_density.sector_size == 256);
    detected = vz256::floppy_geometries::detect(double_density.image_size());
    assert(detected != nullptr);
    assert(detected->name == "8-dsdd-77");
    assert(machine.drive(0).load(temp / "eight-double-density.img"));
    machine.output(0xd1, 76);
    machine.output(0xd2, 26);
    machine.output(0xd0, 0xc2); // Read Address, side compare/side 1
    assert(machine.input(0xd3) == 76);
    machine.tick(64);
    assert(machine.input(0xd3) == 1);
    machine.tick(64);
    assert(machine.input(0xd3) == 26);
    machine.tick(64);
    assert(machine.input(0xd3) == 1); // WD2797 N=1 means 256 bytes

    fs::remove_all(temp);
}
