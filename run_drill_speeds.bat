@echo off
setlocal

set "DRILL_EXE=%~dp0drill\bin\Drill.exe"

if exist "%DRILL_EXE%" goto :run

echo [ERROR] Missing drill executable.
echo [ERROR] Build it with win\build_drill.bat.
endlocal
exit /b 1

:run
"%DRILL_EXE%" "1"
set "DRILL_EXIT=%ERRORLEVEL%"

endlocal & exit /b %DRILL_EXIT%
