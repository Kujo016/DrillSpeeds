#include "builder.h"
#include "utils/version.h"
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <iostream>

//COMPILER TOOL
Builder::CompilerTool compilerTool;
//Set CudaEnabled
bool CudaEnabled =
#ifdef __CUDACC__
    true;
#else
    false;
#endif

//Set WindowsEnabled
bool WindowsEnabled =
#ifdef _WIN32
    true;
#else
    false;
#endif


#ifdef _WIN32
//COMPILER FLAGS
//Set debug build
bool DebugBuild = true;
//Set math error
bool MathError = false;
//Set MSVC debug flags for standard debug
std::string MSVC_DEBUG_FLAGS_STANDARD_DEBUG =
    "/EHsc /Od /MTd /D_DEBUG ";
std::string MSVC_DEBUG_FLAGS_SOLVE_MATH_ERROR =
    "/EHsc /Z7 /Od /MTd /D_DEBUG ";
//Set MSVC debug flags
std::string MSVC_DEBUG_FLAGS = MathError ? MSVC_DEBUG_FLAGS_STANDARD_DEBUG : MSVC_DEBUG_FLAGS_SOLVE_MATH_ERROR;
//Set MSVC release flags for standard release
std::string MSVC_RELEASE_FLAGS =
    "/EHsc /O2 /DNDEBUG /MT ";
//Set CUDA define
std::string cuda_define = CudaEnabled ? " /DUSE_CUDA_TYPES" : "";
//Set COMPILER_CPP
std::string COMPILER_CPP =
    "cl /nologo /std:c++20 " +
    (DebugBuild ? MSVC_DEBUG_FLAGS : MSVC_RELEASE_FLAGS) +
    cuda_define;
#else
//Set COMPILER_CPP for Linux
const std::string COMPILER_CPP =
    "g++ -std=c++20 " +
    std::string(DebugBuild ? "-g -O0 " : "-O2 ");
#endif
//Set CUDA debug flags for standard debug
std::string CUDA_DEBUG_FLAGS_STANDARD_DEBUG = "-g -lineinfo ";
// Includes host debug, fast device profiling, and thread-safe PDB generation
std::string CUDA_DEBUG_FLAGS_SOLVE_MATH_ERROR = "-g -lineinfo -Xcompiler \"/Z7\" ";
//Set CUDA debug flags
std::string CUDA_DEBUG_FLAGS = MathError ? CUDA_DEBUG_FLAGS_STANDARD_DEBUG : CUDA_DEBUG_FLAGS_SOLVE_MATH_ERROR;
//Set CUDA release flags
std::string CUDA_RELEASE_FLAGS = "-O3 ";
//Set COMPILER_CUDA
const std::string COMPILER_CUDA =
    std::string("nvcc -dc -std=c++17 ") +
    (DebugBuild ?  CUDA_DEBUG_FLAGS: CUDA_RELEASE_FLAGS);
//Set LINK_DEBUG
std::string LINK_DEBUG = DebugBuild ? "/DEBUG " : "";
//Set NVCC_LINK_DEBUG
std::string NVCC_LINK_DEBUG =
    DebugBuild ? "-Xlinker \"/DEBUG\"" : "";



#ifdef _WIN32
//SET BUILDER PATHS AND NAMES
//Set Builder_Windows namespace
using namespace Builder_Windows;
//Get project directory
FileTool fileTool = FileTool();
const std::string PROJ_DIR = fileTool.GetProjectPath().parent_path().parent_path().string();
//Get project name
const auto projectFolder = std::filesystem::path(PROJ_DIR).filename().string();
//Get project name in lowercase and set as object name  
const std::string PROJECT_NAME = ExtractName(projectFolder);
const std::string OBJ_NAME = ExtractName_to_lower(projectFolder);
const auto PATH_TO_BUILDER_CPP =
    std::filesystem::path(PROJ_DIR) / "builder" / "FileTool_Windows.cpp";

#else
using namespace Builder_Linux;
//Get project directory
FileTool fileTool = FileTool();
const std::string PROJ_DIR = fileTool.GetProjectPath().string();
//Get project name
auto projectFolder = std::filesystem::path(PROJ_DIR).filename().string();
//Get project name and set as object name
const std::string PROJECT_NAME    = ExtractName(projectFolder);
//Get project name in lowercase and set as object name
const std::string OBJ_NAME        = ExtractName_to_lower(projectFolder);
const auto PATH_TO_BUILDER_CPP =
    std::filesystem::path(PROJ_DIR) / "builder" / "FileTool_Linux.cpp";
#endif

//SET INCLUDES AND EXTENSIONS BASED ON OS
#ifdef _WIN32
//CUDA CHECK
#ifdef __CUDACC__
static std::filesystem::path GetCudaToolkitRoot() {
    const char* cudaPath = std::getenv("CUDA_PATH");
    if (cudaPath && *cudaPath) {
        return std::filesystem::path(cudaPath);
    }

    return {};
}

static std::string GetCudaIncludeFlags() {
    const auto root = GetCudaToolkitRoot();
    if (root.empty()) {
        return "";
    }

    const auto includeDir = root / "include";
    if (!std::filesystem::exists(includeDir)) {
        return "";
    }

    return "-I\"" + includeDir.string() + "\" ";
}

static std::string GetCudaToolkitLibDir() {
    const auto root = GetCudaToolkitRoot();
    if (root.empty()) {
        return "";
    }

    const auto libDir = root / "lib" / "x64";
    if (!std::filesystem::exists(libDir)) {
        return "";
    }

    return libDir.string();
}
#endif
//END CUDA CHECK
//Set object extension
const std::string OBJ_EXT = ".obj";
const std::string EXE_EXT = ".exe";
const std::string DLL_EXT = ".dll";

//HELPER FUNCTIONS
//Quote function for returning slash escaped strings
static std::string Quote(const std::string& value) {
    return "\"" + value + "\"";
}
//Quote function for returning slash escaped filesystem paths
static std::string Quote(const std::filesystem::path& value) {
    return Quote(value.string());
}
//Gather objects function for returning a string of all objects in a directory
static std::string GatherObjs(const std::string& objDir) {
    std::string out;
    for (const auto& e : std::filesystem::directory_iterator(objDir)) {
        if (e.is_regular_file() && e.path().extension() == OBJ_EXT) {
            out += Quote(e.path()) + " ";
        }
    }
    return out;
}
//END HELPER FUNCTIONS
//ELSE LINUX
#else
//CUDA CHECK
#ifdef __CUDACC__
static std::string GetCudaIncludeFlags() {
    const char* cudaPath = std::getenv("CUDA_PATH");
    if (cudaPath && *cudaPath) {
        const auto includeDir = std::filesystem::path(cudaPath) / "include";
        if (std::filesystem::exists(includeDir)) {
            return "-I\"" + includeDir.string() + "\" ";
        }
    }

    if (std::filesystem::exists("/usr/local/cuda/include")) {
        return "-I\"/usr/local/cuda/include\" ";
    }

    return "";
}

static std::string GetCudaLibFlags() {
    const char* cudaPath = std::getenv("CUDA_PATH");
    if (cudaPath && *cudaPath) {
        const auto libDir = std::filesystem::path(cudaPath) / "lib64";
        if (std::filesystem::exists(libDir)) {
            return "-L\"" + libDir.string() + "\" ";
        }
    }

    if (std::filesystem::exists("/usr/local/cuda/lib64")) {
        return "-L\"/usr/local/cuda/lib64\" ";
    }

    return "";
}
#endif
//END CUDA CHECK

//Set object extension
const std::string OBJ_EXT = ".o";
const std::string EXE_EXT = "";

#endif

//END ELSE LINUX

struct BuildOptions {
    std::vector<std::filesystem::path> targets;
    std::vector<std::filesystem::path> includeDirs;
};

static std::string BuildExeName(const std::filesystem::path& targetDir) {
    std::string name = targetDir.filename().string();
    if (name.empty()) {
        return PROJECT_NAME;
    }

    name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    return name;
}

static std::filesystem::path ResolveTargetDir(const std::string& arg) {
    std::filesystem::path target(arg);
    if (target.is_absolute()) {
        return target.lexically_normal();
    }

    return (std::filesystem::path(PROJ_DIR) / target).lexically_normal();
}

static std::filesystem::path ResolveProjectPath(const std::string& arg) {
    std::filesystem::path path(arg);
    if (path.is_absolute()) {
        return path.lexically_normal();
    }

    return (std::filesystem::path(PROJ_DIR) / path).lexically_normal();
}

static std::string IncludeFlag(const std::filesystem::path& includeDir) {
    #ifdef _WIN32
        return "-I\"" + includeDir.string() + "\" ";
    #else
        return "-I\"" + includeDir.string() + "\" ";
    #endif
}

static std::string BuildIncludeFlags(
    const std::filesystem::path& targetDir,
    const std::vector<std::filesystem::path>& includeDirs
) {
    std::string flags;
    flags += IncludeFlag(targetDir / "include");

    for (const auto& includeDir : includeDirs) {
        flags += IncludeFlag(includeDir);
    }

    return flags;
}

static bool IsBuildableTarget(const std::filesystem::path& targetDir) {
    return std::filesystem::exists(targetDir / "src") &&
           std::filesystem::exists(targetDir / "include");
}

static std::filesystem::path ResolveSdlImportLibDir(const std::filesystem::path& targetDir) {
    const auto targetLib = targetDir / "lib";
    if (std::filesystem::exists(targetLib / "SDL3.lib")) {
        return targetLib;
    }

    return std::filesystem::path(PROJ_DIR) / "system" / "lib";
}

static std::filesystem::path ResolveRuntimeDllDir(const std::filesystem::path& targetDir) {
#ifdef _WIN32
    const char* configuredDir = std::getenv("VECTORISO_RUNTIME_DLL_DIR");
    if (configuredDir && *configuredDir) {
        return std::filesystem::path(configuredDir).lexically_normal();
    }
#endif

    const auto targetDllDir = targetDir / "dll";
    if (std::filesystem::exists(targetDllDir)) {
        return targetDllDir;
    }

    return std::filesystem::path(PROJ_DIR) / "system" / "dll";
}

static std::string ToLower(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

static bool UsesSdl(const std::filesystem::path& targetDir) {
    if (std::filesystem::exists(targetDir / "lib" / "SDL3.lib")) {
        return true;
    }

    return ToLower(targetDir.filename().string()) ==  "system" && std::filesystem::exists(std::filesystem::path(PROJ_DIR) / "lib" / "SDL3.lib");
}

static bool HasPyQtDllEntry(const std::filesystem::path& targetDir) {
#ifdef _WIN32
    return std::filesystem::exists(targetDir / "src" / "mainPyQt6.cpp");
#else
    return false;
#endif
}

#ifdef _WIN32
static bool StageRuntimeDll(
    const std::filesystem::path& runtimeDllDir,
    const std::filesystem::path& binDir,
    const std::string& dllName
) {
    const auto source = runtimeDllDir / dllName;
    const auto destination = binDir / dllName;

    if (!std::filesystem::exists(source)) {
        std::cerr << "[ERROR] Missing local runtime DLL: " << source << "\n";
        return false;
    }

    std::error_code copyError;
    std::filesystem::copy_file(
        source,
        destination,
        std::filesystem::copy_options::overwrite_existing,
        copyError
    );

    if (copyError) {
        std::cerr << "[ERROR] Failed to stage " << source << " to "
                  << destination << ": " << copyError.message() << "\n";
        return false;
    }

    std::cout << "Staged runtime DLL: " << destination << "\n";
    return true;
}
#endif

static int BuildTarget(const std::filesystem::path& targetDir, const std::vector<std::filesystem::path>& includeDirs) {
    if (!IsBuildableTarget(targetDir)) {
        std::cerr << "[ERROR] Build target must contain src and include folders: " << targetDir << "\n";
        return 1;
    }

    const std::string exeName = BuildExeName(targetDir);
    const std::string srcDir = (targetDir / "src").string();
    const std::string includeDir = (targetDir / "include").string();
    const std::string objDir = (targetDir / "obj").string();
    const bool usesSdl = UsesSdl(targetDir);
    const std::string libDir = usesSdl ? ResolveSdlImportLibDir(targetDir).string() : "";
    const auto runtimeDllDir = ResolveRuntimeDllDir(targetDir);
    const auto binPath = targetDir / "bin";
    const std::string binDir = binPath.string();
	
	
    const auto exe = std::filesystem::path(binDir) / (exeName + EXE_EXT);
    const std::string projectIncludes = BuildIncludeFlags(targetDir, includeDirs);

    std::cout << "Building target: " << targetDir << "\n";
    std::cout << "BIN_DIR: " << binDir << "\n";
    std::cout << "OBJ_DIR: " << objDir << "\n";
    std::cout << "RUNTIME_DLL_DIR: " << runtimeDllDir << "\n";
    fileTool.CreateDirectory({objDir, binDir});	

#ifdef _WIN32
    if (usesSdl && !std::filesystem::exists(runtimeDllDir / "SDL3.dll")) {
        std::cerr << "[ERROR] SDL3.dll must exist in the local runtime directory: "
                  << runtimeDllDir << "\n";
        return 1;
    }

    #ifdef __CUDACC__
    if (!std::filesystem::exists(runtimeDllDir / "cudart64_12.dll")) {
        std::cerr << "[ERROR] cudart64_12.dll must exist in the local runtime directory: "
                  << runtimeDllDir << "\n";
        return 1;
    }
    #endif
#endif

    std::cout << "Begin compiling..." << exeName << "\n";
    #ifndef _WIN32
        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
            if (entry.path().extension() == ".cpp") {
                std::cout << " Compiling " << entry.path() << "\n";
                compilerTool.CompileSourceFiles_CPP(
                    entry.path(),
					PATH_TO_BUILDER_CPP,
                    projectIncludes,
                    objDir,
                    OBJ_EXT,
                    COMPILER_CPP
                );
                std::cout << " Compiled " << entry.path() << "\n";
            }
        }
    #else
        #ifdef __CUDACC__
            const std::string cppIncludes = GetCudaIncludeFlags() + projectIncludes;
        #else
            const std::string cppIncludes = projectIncludes;
        #endif
        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
            if (entry.path().extension() == ".cpp" || entry.path().extension() == ".c") {
                std::cout << " Compiling " << entry.path() << "\n";
                compilerTool.CompileSourceFiles_CPP(
                    entry.path(),
					PATH_TO_BUILDER_CPP,
                    cppIncludes,
                    objDir,
                    OBJ_EXT,
                    COMPILER_CPP
                );
                std::cout << " Compiled " << entry.path() << "\n";
            }
        }
    #endif

    #ifdef __CUDACC__
    for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
        if (entry.path().extension() == ".cu") {
            #ifdef _WIN32
            const std::string cudaCompileFlags =
                GetCudaIncludeFlags() +
                projectIncludes +
                "-Xcompiler \"" +
                std::string(DebugBuild ? MSVC_DEBUG_FLAGS : MSVC_RELEASE_FLAGS) +
                "\" ";
            #else
            const std::string cudaCompileFlags =
                GetCudaIncludeFlags() +
                projectIncludes;
            #endif
            std::cout << " Compiling " << entry.path() << "\n";
            compilerTool.CompileSourceFiles_CUDA(
                entry.path(),
				PATH_TO_BUILDER_CPP,
                cudaCompileFlags,
                objDir,
                OBJ_EXT,
                COMPILER_CUDA
            );
            std::cout << " Compiled " << entry.path() << "\n";
        }
    }
    #endif

    std::cout << "Linking " << targetDir << "\n";
    #ifndef _WIN32
        #ifdef __CUDACC__
            compilerTool.RunCommand(
                "nvcc \"" + objDir + "\"/*.o "
                + std::string(usesSdl ? "-L\"" + libDir + "\" " : "") +
                GetCudaLibFlags() +
                "-lcudart -luser32 -lgdi32 -lopengl32 " +
                std::string(usesSdl ? "-lSDL3 " : "") +
                "-Xlinker -rpath -Xlinker '$ORIGIN/lib' "
                "-o \"" + exe.string() + "\""
            );
        #else
            compilerTool.RunCommand(
                "g++ \"" + objDir + "\"/*.o "
                + std::string(usesSdl ? "-L\"" + libDir + "\" -lSDL3 " : "") +
                "-Wl,-rpath,'$ORIGIN/lib' "
                "-o \"" + exe.string() + "\""
            );
        #endif
    #else
		std::string objs = GatherObjs(objDir);
        #ifdef __CUDACC__
			const std::string cudaLibDir = GetCudaToolkitLibDir();

			if (cudaLibDir.empty()) {
				std::cerr << "[ERROR] CUDA_PATH must point to a CUDA toolkit with lib\\x64 when CUDA is enabled.\n";
				return 1;
			}

			const auto pdb =
				std::filesystem::path(binDir) /
				(exeName + ".pdb");

			compilerTool.RunCommand(
				"nvcc " + objs +
				"-o " + Quote(exe) + " "
				+ std::string(usesSdl ? "-L" + Quote(libDir) + " " : "") +
				"-L" + Quote(cudaLibDir) + " "
				"-lcudart user32.lib gdi32.lib opengl32.lib " +
				std::string(usesSdl ? "SDL3.lib " : "") +
				"-Xcompiler \"" + std::string(DebugBuild ? MSVC_DEBUG_FLAGS : MSVC_RELEASE_FLAGS) + "\" " +
				(DebugBuild
					? "-Xlinker \"/DEBUG\" -Xlinker \"/PDB:\\\"" + pdb.string() + "\\\"\" "
					: "")
			);
		#else				
            compilerTool.RunCommand(
                "cl " + objs +
                " /Fe:" + Quote(exe) + " " +
                (DebugBuild ? MSVC_DEBUG_FLAGS : MSVC_RELEASE_FLAGS) +
                " /link " + LINK_DEBUG +
                std::string(usesSdl ? " /LIBPATH:" + Quote(libDir) + " SDL3.lib" : "") +
                " user32.lib"
            );
        #endif

        if (HasPyQtDllEntry(targetDir)) {
            const auto dll =
                std::filesystem::path(binDir) /
                (exeName + "PyQt6" + DLL_EXT);
            const auto dllPdb =
                std::filesystem::path(binDir) /
                (exeName + "PyQt6.pdb");
            const auto importLib =
                std::filesystem::path(binDir) /
                (exeName + "PyQt6.lib");
            std::error_code removeError;
            std::filesystem::remove(dll, removeError);

            std::cout << "Linking PyQt6 DLL: " << dll << "\n";

            #ifdef __CUDACC__
                compilerTool.RunCommand(
                    "nvcc -shared " + objs +
                    "-o " + Quote(dll) + " "
                    + std::string(usesSdl ? "-L" + Quote(libDir) + " " : "") +
                    "-L" + Quote(cudaLibDir) + " "
                    "-lcudart user32.lib gdi32.lib opengl32.lib " +
                    std::string(usesSdl ? "SDL3.lib " : "") +
                    "-Xcompiler \"" + std::string(DebugBuild ? MSVC_DEBUG_FLAGS : MSVC_RELEASE_FLAGS) + "\""
                );
            #else
                compilerTool.RunCommand(
                    "cl " + objs +
                    " /LD /Fe:" + Quote(dll) + " " +
                    (DebugBuild ? MSVC_DEBUG_FLAGS : MSVC_RELEASE_FLAGS) +
                    " /link " + LINK_DEBUG +
                    " /PDB:" + Quote(dllPdb) +
                    " /IMPLIB:" + Quote(importLib) +
                    std::string(usesSdl ? " /LIBPATH:" + Quote(libDir) + " SDL3.lib" : "") +
                    " user32.lib gdi32.lib opengl32.lib"
                );
            #endif

            if (!std::filesystem::exists(dll)) {
                std::cerr << "[ERROR] PyQt6 DLL was not created: " << dll << "\n";
                return 1;
            }
        }

        if (usesSdl && !StageRuntimeDll(runtimeDllDir, binPath, "SDL3.dll")) {
            return 1;
        }

        #ifdef __CUDACC__
        if (!StageRuntimeDll(runtimeDllDir, binPath, "cudart64_12.dll")) {
            return 1;
        }
        #endif
    #endif

    return 0;
}

static BuildOptions ParseArgs(int argc, char* argv[]) {
    BuildOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--include" || arg == "-I" || arg == "/I") {
            if (i + 1 >= argc) {
                throw std::runtime_error(arg + " requires a path argument");
            }
            options.includeDirs.push_back(ResolveProjectPath(argv[++i]));
            continue;
        }

        const std::string includePrefix = "--include=";
        if (arg.rfind(includePrefix, 0) == 0) {
            options.includeDirs.push_back(ResolveProjectPath(arg.substr(includePrefix.size())));
            continue;
        }

        if ((arg.rfind("-I", 0) == 0 || arg.rfind("/I", 0) == 0) && arg.size() > 2) {
            options.includeDirs.push_back(ResolveProjectPath(arg.substr(2)));
            continue;
        }

        options.targets.push_back(ResolveTargetDir(arg));
    }

    if (options.targets.empty()) {
        options.targets.push_back(ResolveTargetDir("system"));
    }

    return options;
}

//MAIN FUNCTION
int main(int argc, char* argv[]) {
    //Print build information
    std::cout << "Building " << (WindowsEnabled ? "Windows" : "Linux") << "\n";
    std::cout << "CudaEnabled: " << (CudaEnabled ? "Yes" : "No") << "\n";

    const auto options = ParseArgs(argc, argv);
    for (const auto& targetDir : options.targets) {
        const int result = BuildTarget(targetDir, options.includeDirs);
        if (result != 0) {
            return result;
        }
    }

    return 0;
}
