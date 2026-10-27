#include "recent_media.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
#include <system_error>

namespace {
constexpr const char* magic = "VZ256-RECENT 1";

std::filesystem::path normalized(const std::filesystem::path& path) {
    std::error_code error;
    auto result = std::filesystem::absolute(path, error);
    if (error) result = path;
    return result.lexically_normal();
}
} // namespace

RecentMedia::RecentMedia(std::filesystem::path storage_path, std::size_t limit)
    : storage_path_(std::move(storage_path)), limit_(limit) {
    load();
}

const std::vector<std::filesystem::path>& RecentMedia::items(Kind kind) const {
    return kind == Kind::disk ? disks_ : tapes_;
}

std::vector<std::filesystem::path>& RecentMedia::mutable_items(Kind kind) {
    return kind == Kind::disk ? disks_ : tapes_;
}

void RecentMedia::add(Kind kind, const std::filesystem::path& path) {
    if (path.empty() || limit_ == 0) return;
    auto value = normalized(path);
    auto& list = mutable_items(kind);
    list.erase(std::remove(list.begin(), list.end(), value), list.end());
    list.insert(list.begin(), std::move(value));
    if (list.size() > limit_) list.resize(limit_);
    save();
}

void RecentMedia::remove(Kind kind, const std::filesystem::path& path) {
    auto& list = mutable_items(kind);
    const auto value = normalized(path);
    list.erase(std::remove(list.begin(), list.end(), value), list.end());
    save();
}

void RecentMedia::clear(Kind kind) {
    mutable_items(kind).clear();
    save();
}

void RecentMedia::load() {
    std::ifstream input(storage_path_);
    std::string line;
    if (!input || !std::getline(input, line) || line != magic) return;
    char kind{};
    std::string value;
    while (input >> kind >> std::quoted(value)) {
        if (kind != 'D' && kind != 'T') continue;
        auto& list = mutable_items(kind == 'D' ? Kind::disk : Kind::tape);
        const auto path = normalized(value);
        if (std::find(list.begin(), list.end(), path) == list.end() && list.size() < limit_)
            list.push_back(path);
    }
}

void RecentMedia::save() const {
    if (storage_path_.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(storage_path_.parent_path(), error);
    if (error) return;
    const auto temporary = storage_path_.string() + ".tmp";
    std::ofstream output(temporary, std::ios::trunc);
    if (!output) return;
    output << magic << '\n';
    for (const auto& path : disks_) output << "D " << std::quoted(path.string()) << '\n';
    for (const auto& path : tapes_) output << "T " << std::quoted(path.string()) << '\n';
    output.close();
    if (!output) { std::filesystem::remove(temporary, error); return; }
    std::filesystem::rename(temporary, storage_path_, error);
    if (error) {
        std::filesystem::remove(storage_path_, error);
        error.clear();
        std::filesystem::rename(temporary, storage_path_, error);
    }
}
