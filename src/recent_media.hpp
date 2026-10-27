#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

class RecentMedia {
public:
    enum class Kind { disk, tape };

    explicit RecentMedia(std::filesystem::path storage_path, std::size_t limit = 10);

    [[nodiscard]] const std::vector<std::filesystem::path>& items(Kind kind) const;
    void add(Kind kind, const std::filesystem::path& path);
    void remove(Kind kind, const std::filesystem::path& path);
    void clear(Kind kind);

private:
    void load();
    void save() const;
    [[nodiscard]] std::vector<std::filesystem::path>& mutable_items(Kind kind);

    std::filesystem::path storage_path_;
    std::size_t limit_;
    std::vector<std::filesystem::path> disks_;
    std::vector<std::filesystem::path> tapes_;
};
