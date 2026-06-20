@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo ===================================================
echo [DEBUG BUILDER] Starting verbose builder build
echo ===================================================

rem ---------------------------------------------------
rem Builder directory
rem This BAT lives inside /builder next to main.cpp
rem ---------------------------------------------------
set "BUILDER_DIR=%~dp0"
cd /d "%BUILDER_DIR%"

echo [DEBUG] Current directory:
echo   %CD%
echo.

rem ---------------------------------------------------
rem Engine root is one folder above builder/
rem ---------------------------------------------------
set "ENGINE_DIR=%BUILDER_DIR%.."
for %%I in ("%ENGINE_DIR%") do set "ENGINE_DIR=%%~fI"

echo [DEBUG] Engine root:
echo   %ENGINE_DIR%
echo.

rem ---------------------------------------------------
rem Output folders
rem ---------------------------------------------------
set "BUILDER_BIN_DIR=%BUILDER_DIR%bin"
set "BUILDER_OBJ_DIR=%BUILDER_DIR%obj"
set "BUILDER_EXE=%BUILDER_BIN_DIR%\builder.exe"

echo [DEBUG] Builder bin:
echo   %BUILDER_BIN_DIR%
echo [DEBUG] Builder obj:
echo   %BUILDER_OBJ_DIR%
echo [DEBUG] Builder exe:
echo   %BUILDER_EXE%
echo.

rem ---------------------------------------------------
rem Clean builder outputs only
rem ---------------------------------------------------
echo [DEBUG] Cleaning builder outputs...

if exist "%BUILDER_BIN_DIR%" (
    echo [CLEAN] %BUILDER_BIN_DIR%
    rmdir /s /q "%BUILDER_BIN_DIR%"
)

if exist "%BUILDER_OBJ_DIR%" (
    echo [CLEAN] %BUILDER_OBJ_DIR%
    rmdir /s /q "%BUILDER_OBJ_DIR%"
)

mkdir "%BUILDER_BIN_DIR%"
mkdir "%BUILDER_OBJ_DIR%"

echo [DEBUG] Clean complete.
echo.

rem ---------------------------------------------------
rem Visual Studio prerequisite
rem ---------------------------------------------------
echo [DEBUG] Searching for Visual Studio vcvars64.bat...

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :FOUND_VS

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :FOUND_VS

set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VS_PATH%" goto :FOUND_VS

echo [ERROR] Could not find vcvars64.bat.
echo [ERROR] Install Visual Studio with C++ workload.
pause
exit /b 1

:FOUND_VS
echo [DEBUG] Found VS:
echo   %VS_PATH%
echo.

echo [DEBUG] Calling vcvars64.bat...
call "%VS_PATH%"

if errorlevel 1 (
    echo [ERROR] Failed to initialize Visual Studio environment.
    pause
    exit /b 1
)

echo.
echo [DEBUG] Visual Studio environment initialized.
echo.

rem ---------------------------------------------------
rem Tool diagnostics
rem ---------------------------------------------------
echo [DEBUG] Checking cl:
where cl
echo.

echo [DEBUG] Checking link:
where link
echo.

echo [DEBUG] Checking nvcc:
where nvcc
echo.

rem ---------------------------------------------------
rem CUDA flag detection
rem ---------------------------------------------------
set "BUILD_FLAGS="

where nvcc >nul 2>&1
if errorlevel 1 (
    echo [DEBUG] CUDA not found. Builder will compile without /D__CUDACC__.
) else (
    echo [DEBUG] CUDA found. Builder will compile with /D__CUDACC__.
    set "BUILD_FLAGS=/D__CUDACC__"
)

echo [DEBUG] BUILD_FLAGS:
echo   %BUILD_FLAGS%
echo.

rem ---------------------------------------------------
rem Source file check
rem ---------------------------------------------------
echo [DEBUG] Checking source files...

set "SRC_MAIN=main.cpp"
set "SRC_FILETOOL=FileTool_Windows.cpp"
set "SRC_COMPILER=CompilerTool.cpp"
set "SRC_VERSION=utils\version.cpp"

for %%F in (
    "%SRC_MAIN%"
    "%SRC_FILETOOL%"
    "%SRC_COMPILER%"
    "%SRC_VERSION%"
) do (
    if exist %%~F (
        echo [FOUND] %%~F
    ) else (
        echo [MISSING] %%~F
        pause
        exit /b 1
    )
)

echo.

rem ---------------------------------------------------
rem Compile builder objects
rem ---------------------------------------------------
echo ===================================================
echo [DEBUG] Compiling builder objects
echo ===================================================

echo cl /nologo /EHsc /std:c++20 /Z7 /Od /MTd /D_DEBUG %BUILD_FLAGS% ^
 /c ^
 /Fo"%BUILDER_OBJ_DIR%\\" ^
 "%SRC_MAIN%" ^
 "%SRC_FILETOOL%" ^
 "%SRC_COMPILER%" ^
 "%SRC_VERSION%"
echo.

cl /nologo /EHsc /std:c++20 /Z7 /Od /MTd /D_DEBUG %BUILD_FLAGS% ^
    /c ^
    /Fo"%BUILDER_OBJ_DIR%\\" ^
    "%SRC_MAIN%" ^
    "%SRC_FILETOOL%" ^
    "%SRC_COMPILER%" ^
    "%SRC_VERSION%"

if errorlevel 1 (
    echo.
    echo [ERROR] Failed to compile builder objects.
    pause
    exit /b 1
)

echo.
echo [DEBUG] Compile succeeded.
echo.

rem ---------------------------------------------------
rem List generated objects
rem ---------------------------------------------------
echo [DEBUG] Generated object files:
dir "%BUILDER_OBJ_DIR%\*.obj"
echo.

rem ---------------------------------------------------
rem Link builder
rem ---------------------------------------------------
echo ===================================================
echo [DEBUG] Linking builder.exe
echo ===================================================

echo link /nologo ^
 "%BUILDER_OBJ_DIR%\main.obj" ^
 "%BUILDER_OBJ_DIR%\FileTool_Windows.obj" ^
 "%BUILDER_OBJ_DIR%\CompilerTool.obj" ^
 "%BUILDER_OBJ_DIR%\version.obj" ^
 /DEBUG ^
 /PDB:"%BUILDER_BIN_DIR%\builder.pdb" ^
 /OUT:"%BUILDER_EXE%"
echo.

link /nologo ^
    "%BUILDER_OBJ_DIR%\main.obj" ^
    "%BUILDER_OBJ_DIR%\FileTool_Windows.obj" ^
    "%BUILDER_OBJ_DIR%\CompilerTool.obj" ^
    "%BUILDER_OBJ_DIR%\version.obj" ^
    /DEBUG ^
    /PDB:"%BUILDER_BIN_DIR%\builder.pdb" ^
    /OUT:"%BUILDER_EXE%"

if errorlevel 1 (
    echo.
    echo [ERROR] Failed to link builder.
    pause
    exit /b 1
)

echo.
echo [DEBUG] Link succeeded.
echo.

rem ---------------------------------------------------
rem Final output check
rem ---------------------------------------------------
if exist "%BUILDER_EXE%" (
    echo [SUCCESS] Builder executable created:
    echo   %BUILDER_EXE%
) else (
    echo [ERROR] builder.exe was not created.
    pause
    exit /b 1
)

echo.
echo [DEBUG] Builder folder after build:
dir "%BUILDER_DIR%"

echo.
echo ===================================================
echo [DEBUG BUILDER] Done
echo ===================================================

pause
endlocal
exit /b 0