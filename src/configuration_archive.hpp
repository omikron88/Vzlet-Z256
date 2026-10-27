#pragma once

#include <filesystem>
#include <string>

bool export_configuration(const std::filesystem::path& configuration_directory,
                          const std::filesystem::path& archive, std::string& error);
bool import_configuration(const std::filesystem::path& configuration_directory,
                          const std::filesystem::path& archive, std::string& error);
