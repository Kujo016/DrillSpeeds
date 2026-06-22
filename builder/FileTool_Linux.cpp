#include "builder.h"

#include <filesystem>
#include <stdexcept>


namespace Builder_Linux {

std::filesystem::path FileTool::GetProjectPath()
{
    char exe_path[4096];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

    if (len == -1) {
        throw std::runtime_error("Failed to get executable path");
    }

    exe_path[len] = '\0';

    return std::filesystem::path(exe_path)
        .parent_path()   // bin/
        .parent_path();  // Vectoriso/
}


std::filesystem::path FileTool::ResolvePathFromExecutable(
    const std::string& relativePath
) {
    std::filesystem::path p(relativePath);

    if (p.is_absolute()) {
        return p;
    }

    return GetProjectPath() / p;
}

void FileTool::CreateDirectory(const std::vector<std::string>& directories)
{
    for (const auto& directory : directories) {
        std::filesystem::path resolved_path =
            ResolvePathFromExecutable(directory);

        std::error_code ec;
        std::filesystem::create_directories(resolved_path, ec);

        if (ec) {
            throw std::runtime_error(
                "Failed to create directory '" +
                resolved_path.string() + "': " +
                ec.message()
            );
        } else {
            std::cout << "Directory ready: "
                      << resolved_path << std::endl;
        }
    }
}
}// namespace Builder_Linux

