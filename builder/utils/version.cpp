#include "version.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <regex>

#ifndef _WIN32
    #include <limits.h>
    #include <unistd.h>
#else
    #include <windows.h>
#endif

std::filesystem::path GetExecutableDir()
{
#ifndef _WIN32
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        return std::filesystem::current_path();
    }
    buffer[len] = '\0';
    std::filesystem::path exePath(buffer);
#else
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
#endif

    std::cout << "[INFO] Executable path: " << exePath << "\n";
    return exePath.parent_path();
}

std::string ExtractVersion(const std::string& folderName)
{
    std::regex pattern(R"(([A-Za-z]+)-(\d+\.\d+\.\d+)-([A-Za-z]+))");
    std::smatch match;

    if (std::regex_search(folderName, match, pattern)) {
        return match[1].str() + "-" + match[2].str();
    }

    return "unknown";
}

std::string ExtractName(const std::string& folderName)
{
    std::regex pattern(R"(([A-Za-z]+)-(\d+\.\d+\.\d+)-([A-Za-z]+))");
    std::smatch match;

    if (std::regex_search(folderName, match, pattern)) {
        return match[1].str();
    }

    return "unknown";
}

std::string ExtractName_to_lower(const std::string& folderName)
{
    std::string name = ExtractName(folderName);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}
