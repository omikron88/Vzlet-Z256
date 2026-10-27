#include "configuration_archive.hpp"

#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <map>
#include <string_view>
#include <vector>

namespace {
constexpr std::string_view magic{"VZ256-CONFIG 1"};
constexpr std::array<std::string_view, 11> filenames{
    "view.cfg", "shortcuts.cfg", "main.window", "about.window", "debugger.window",
    "media.window", "profiles.window", "recent-media.cfg", "serial.window",
    "shortcuts.window", "tape.window"};

bool allowed(std::string_view name) {
    for (const auto candidate : filenames)
        if (candidate == name) return true;
    return false;
}

char hex_digit(unsigned value) {
    return static_cast<char>(value < 10 ? '0' + value : 'a' + value - 10);
}

std::string encode(const std::vector<unsigned char>& data) {
    std::string result;
    result.reserve(data.size() * 2);
    for (const auto byte : data) {
        result.push_back(hex_digit(byte >> 4));
        result.push_back(hex_digit(byte & 0x0f));
    }
    return result;
}

int hex_value(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    return value >= 'a' && value <= 'f' ? value - 'a' + 10 : -1;
}

bool decode(std::string_view text, std::vector<unsigned char>& data) {
    if (text.size() % 2 != 0 || text.size() > 2 * 1024 * 1024) return false;
    data.clear();
    data.reserve(text.size() / 2);
    for (std::size_t i = 0; i < text.size(); i += 2) {
        const int high = hex_value(text[i]);
        const int low = hex_value(text[i + 1]);
        if (high < 0 || low < 0) return false;
        data.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return true;
}
} // namespace

bool export_configuration(const std::filesystem::path& configuration_directory,
                          const std::filesystem::path& archive, std::string& error) {
    std::ofstream output(archive, std::ios::binary | std::ios::trunc);
    if (!output) { error = "cannot create configuration archive"; return false; }
    output << magic << '\n';
    for (const auto name : filenames) {
        std::ifstream input(configuration_directory / name, std::ios::binary);
        if (!input) continue;
        const std::vector<unsigned char> data{std::istreambuf_iterator<char>(input), {}};
        output << name << ' ' << encode(data) << '\n';
    }
    if (!output) { error = "cannot write configuration archive"; return false; }
    return true;
}

bool import_configuration(const std::filesystem::path& configuration_directory,
                          const std::filesystem::path& archive, std::string& error) {
    std::ifstream input(archive, std::ios::binary);
    std::string line;
    if (!input || !std::getline(input, line) || line != magic) {
        error = "not a Vzlet Z-256 configuration archive";
        return false;
    }
    std::map<std::string, std::vector<unsigned char>> files;
    while (std::getline(input, line)) {
        const auto separator = line.find(' ');
        if (separator == std::string::npos) { error = "invalid archive entry"; return false; }
        const auto name = line.substr(0, separator);
        std::vector<unsigned char> data;
        if (!allowed(name) || files.contains(name) ||
            !decode(std::string_view(line).substr(separator + 1), data)) {
            error = "invalid or duplicate archive entry";
            return false;
        }
        files.emplace(name, std::move(data));
    }
    if (!input.eof()) { error = "cannot read configuration archive"; return false; }
    std::error_code filesystem_error;
    std::filesystem::create_directories(configuration_directory, filesystem_error);
    if (filesystem_error) { error = "cannot create configuration directory"; return false; }
    for (const auto& [name, data] : files) {
        std::ofstream output(configuration_directory / name, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(data.data()),
                     static_cast<std::streamsize>(data.size()));
        if (!output) { error = "cannot write imported configuration"; return false; }
    }
    return true;
}
