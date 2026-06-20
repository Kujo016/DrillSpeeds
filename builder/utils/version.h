#pragma once
#include <filesystem>
#include <string>


std::filesystem::path GetExecutableDir();
std::string ExtractVersion(const std::string& folderName);
std::string ExtractName(const std::string& folderName);
std::string ExtractName_to_lower(const std::string& folderName);

