#include "recent_media.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() /
        ("vz256-recent-test-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto settings = root / "recent-media.cfg";
    fs::create_directories(root);

    RecentMedia recent(settings, 3);
    recent.add(RecentMedia::Kind::disk, root / "one.img");
    recent.add(RecentMedia::Kind::disk, root / "two with spaces.imd");
    recent.add(RecentMedia::Kind::disk, root / "three.img");
    recent.add(RecentMedia::Kind::disk, root / "one.img");
    recent.add(RecentMedia::Kind::tape, root / "demo.wav");
    assert(recent.items(RecentMedia::Kind::disk).size() == 3);
    assert(recent.items(RecentMedia::Kind::disk).front().filename() == "one.img");

    RecentMedia restored(settings, 3);
    assert(restored.items(RecentMedia::Kind::disk).size() == 3);
    assert(restored.items(RecentMedia::Kind::disk)[1].filename() == "three.img");
    assert(restored.items(RecentMedia::Kind::disk)[2].filename() == "two with spaces.imd");
    assert(restored.items(RecentMedia::Kind::tape).front().filename() == "demo.wav");
    restored.remove(RecentMedia::Kind::disk, root / "three.img");
    assert(restored.items(RecentMedia::Kind::disk).size() == 2);
    restored.clear(RecentMedia::Kind::tape);

    RecentMedia final(settings, 3);
    assert(final.items(RecentMedia::Kind::disk).size() == 2);
    assert(final.items(RecentMedia::Kind::tape).empty());
    fs::remove_all(root);
}
