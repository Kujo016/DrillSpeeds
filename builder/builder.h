#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif




namespace Builder_Windows {
    class FileTool {
    public:
        void CreateDirectory(const std::vector<std::string>& directories);
        std::filesystem::path GetProjectPath();
        std::filesystem::path ResolvePathFromExecutable(const std::string& relativePath);
    };

}
namespace Builder_Linux {
    class FileTool {
    public:
        void CreateDirectory(const std::vector<std::string>& directories);
        std::filesystem::path GetProjectPath();
        std::filesystem::path ResolvePathFromExecutable(const std::string& relativePath);
    };

}

namespace Builder {
    class CompilerTool {
    public:
        void RunCommand(const std::string& command);
        void CompileSourceFiles_CPP(const std::filesystem::path& sourcePath, const std::filesystem::path& pathToBuilder, const std::string& flags, const std::string& OBJ_DIR, const std::string& OBJ_EXT, const std::string& COMPILER_CPP);
        void CompileSourceFiles_CUDA(const std::filesystem::path& sourcePath, const std::filesystem::path& pathToBuilder, const std::string& flags, const std::string& OBJ_DIR, const std::string& OBJ_EXT, const std::string& COMPILER_CUDA);
    };
}