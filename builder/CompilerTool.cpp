#include "builder.h"

namespace Builder {
void CompilerTool::RunCommand(const std::string& command)
{
   std::cout << "Running command: " << command << "\n";
   int result = std::system(command.c_str());

   if (result != 0) {
       throw std::runtime_error("Command failed with exit code " + std::to_string(result));
    }
}

void CompilerTool::CompileSourceFiles_CPP(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& pathToBuilder,
    const std::string& flags,
    const std::string& OBJ_DIR,
    const std::string& OBJ_EXT,
    const std::string& COMPILER_CPP)
{
    if (!pathToBuilder.empty()) {
        std::string builderFilename = pathToBuilder.stem().string();
        std::filesystem::path builderOBJ =
			std::filesystem::path(OBJ_DIR) / (builderFilename + OBJ_EXT);

        if (!std::filesystem::exists(builderOBJ) ||
            std::filesystem::last_write_time(pathToBuilder) >
            std::filesystem::last_write_time(builderOBJ)) {
            std::string builderCmd;
        #ifndef _WIN32
            builderCmd = COMPILER_CPP + " " + flags +
                " -c \"" + pathToBuilder.string() +
                "\" -o \"" + builderOBJ.string() + "\"";
        #else
            builderCmd = COMPILER_CPP + " " + flags +
                " /c \"" + pathToBuilder.string() + "\""
                " /Fo:\"" + builderOBJ.string() + "\"";
        #endif
            RunCommand(builderCmd);
        }
    }

    std::string filename = sourcePath.stem().string();
    std::filesystem::path targetOBJ =
        std::filesystem::path(OBJ_DIR) / (filename + OBJ_EXT);

    if (std::filesystem::exists(targetOBJ) &&
        std::filesystem::last_write_time(sourcePath) <=
        std::filesystem::last_write_time(targetOBJ)) {
        return;
    }

    std::string cmd;
    #ifndef _WIN32
        cmd = COMPILER_CPP + " " + flags +
            " -c \"" + sourcePath.string() +
            "\" -o \"" + targetOBJ.string() + "\"";
    #else
        cmd = COMPILER_CPP + " " + flags +
            " /c \"" + sourcePath.string() + "\" " +
            " /Fo:\"" + targetOBJ.string() + "\"";
    #endif

    RunCommand(cmd);
}
#ifdef _WIN32
void CompilerTool::CompileSourceFiles_CUDA(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& pathToBuilder,
    const std::string& flags,
    const std::string& OBJ_DIR,
    const std::string& OBJ_EXT,
    const std::string& COMPILER_CUDA)
{
    std::string filename = sourcePath.stem().string() + "_cu";
    std::filesystem::path targetOBJ =
        std::filesystem::path(OBJ_DIR) / (filename + OBJ_EXT);

    if (std::filesystem::exists(targetOBJ) &&
        std::filesystem::last_write_time(sourcePath) <=
        std::filesystem::last_write_time(targetOBJ)) {
        return;
    }

    // IMPORTANT: source file must come immediately after -dc
    std::string cmd =
        COMPILER_CUDA + " " +
        "\"" + sourcePath.string() + "\" " +
        flags +
        "-o \"" + targetOBJ.string() + "\"";

    RunCommand(cmd);
}
#else
void CompilerTool::CompileSourceFiles_CUDA(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& pathToBuilder,
    const std::string& flags,
    const std::string& OBJ_DIR,
    const std::string& OBJ_EXT,
    const std::string& COMPILER_CUDA)
{
    std::string filename = sourcePath.stem().string() + "_cu";
    std::filesystem::path targetOBJ =
        std::filesystem::path(OBJ_DIR) / (filename + OBJ_EXT);

    if (std::filesystem::exists(targetOBJ) &&
        std::filesystem::last_write_time(sourcePath) <=
        std::filesystem::last_write_time(targetOBJ)) {
        return;
    }

    // IMPORTANT: source file must come immediately after -dc
    std::string cmd =
        COMPILER_CUDA + " " +
        "\"" + sourcePath.string() + "\" " +
        flags +
        "-o \"" + targetOBJ.string() + "\"";

    RunCommand(cmd);
}
#endif

}
