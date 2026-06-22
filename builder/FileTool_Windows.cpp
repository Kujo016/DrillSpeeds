#include "builder.h"

#include <stdexcept>

namespace Builder_Windows {

    std::filesystem::path FileTool::GetProjectPath() {
        char exe_path[MAX_PATH];
        DWORD len = GetModuleFileName(NULL, exe_path, MAX_PATH);
        if (len == 0 || len == MAX_PATH){
            throw std::runtime_error("Failed to get executable path");
        }        

        return std::filesystem::path(exe_path).parent_path();
    }

    std::filesystem::path FileTool::ResolvePathFromExecutable(const std::string& relativePath) {
        std::filesystem::path p(relativePath);

        if (p.is_absolute()) {
            return p;
        }

        return GetProjectPath() / p;
    }

    void FileTool::CreateDirectory(const std::vector<std::string>& directories) {
        for (const auto& directory : directories) {
            const std::filesystem::path resolved_path = ResolvePathFromExecutable(directory);
            std::error_code ec;
            std::filesystem::create_directories(resolved_path, ec);
            if (ec) {
                throw std::runtime_error("Failed to create directory '" + resolved_path.string() + "': " + ec.message());
            } else {
                std::cout << "Directory ready: " << resolved_path << std::endl;
            }
        }
    }
    
}
