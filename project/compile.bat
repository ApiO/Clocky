@echo off

set WIN32_BUILD_PATH=..\pgtech\platform\win32\build\Debug
set COMPILE=%WIN32_BUILD_PATH%\compile_server.exe cfg: %WIN32_BUILD_PATH%\data sch:  %WIN32_BUILD_PATH%\schemas src: resources data: data"

call %COMPILE%

pause