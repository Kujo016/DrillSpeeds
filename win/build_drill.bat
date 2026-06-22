@echo off
call "%~dp0build.bat" drill^

%*

exit /b %ERRORLEVEL%
