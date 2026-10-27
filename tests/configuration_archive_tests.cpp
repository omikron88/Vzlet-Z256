#include "configuration_archive.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    const auto root = std::filesystem::temp_directory_path() /
        ("vz256-config-test-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto source = root / "source";
    const auto target = root / "target";
    const auto archive = root / "settings.vzconfig";
    std::filesystem::create_directories(source);
    { std::ofstream(source / "view.cfg", std::ios::binary) << "scanlines=1\n"; }
    { std::ofstream(source / "main.window", std::ios::binary) << "10 20 960 720\n"; }
    { std::ofstream(source / "profiles.window", std::ios::binary) << "30 40 560 430\n"; }
    { std::ofstream(source / "recent-media.cfg", std::ios::binary)
          << "VZ256-RECENT 1\nD \"/tmp/disk.img\"\n"; }
    { std::ofstream(source / "ignored.file", std::ios::binary) << "secret\n"; }

    std::string error;
    assert(export_configuration(source, archive, error));
    assert(import_configuration(target, archive, error));
    assert(std::filesystem::exists(target / "view.cfg"));
    assert(std::filesystem::exists(target / "main.window"));
    assert(std::filesystem::exists(target / "profiles.window"));
    assert(std::filesystem::exists(target / "recent-media.cfg"));
    assert(!std::filesystem::exists(target / "ignored.file"));
    std::string view;
    std::getline(std::ifstream(target / "view.cfg"), view);
    assert(view == "scanlines=1");

    { std::ofstream(root / "invalid.vzconfig") << "VZ256-CONFIG 1\n../bad 00\n"; }
    assert(!import_configuration(target, root / "invalid.vzconfig", error));
    assert(!std::filesystem::exists(root / "bad"));
    std::filesystem::remove_all(root);
}
