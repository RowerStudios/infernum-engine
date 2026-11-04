@echo off
setlocal EnableExtensions


pushd "%~dp0" || exit /b 1

set "CXX=g++"
set "OUT=build\windows"
set "EXE=%OUT%\Inferno.exe"
set "SCENE=%OUT%\scenes\main_scene.dll"

set "CXXFLAGS=-std=c++23 -Os -flto -ffunction-sections -fdata-sections -DNDEBUG -DSDL_MAIN_HANDLED"
set "LDFLAGS=-flto -Wl,--gc-sections -Wl,--strip-all -static-libgcc -static-libstdc++"

set "SDL_INCLUDES=-Ithirdparty\windows\SDL3-headers -Ithirdparty\windows\SDL3_ttf-headers -Ithirdparty\windows\SDL3_image-headers"
set "SDL_DLLS=thirdparty\windows\SDL3\SDL3.dll thirdparty\windows\SDL3_ttf\SDL3_ttf.dll thirdparty\windows\SDL3_image\SDL3_image.dll"

set "ENGINE_SOURCES=engine\cpp\sounds.cpp engine\cpp\engine.cpp"
if exist "engine\cpp\tween.cpp" set "ENGINE_SOURCES=%ENGINE_SOURCES% engine\cpp\tween.cpp"

where "%CXX%" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] MinGW-w64 g++ not found on PATH.
    goto :fail
)

for %%F in ("thirdparty\windows\SDL3\SDL3.dll" "thirdparty\windows\SDL3_ttf\SDL3_ttf.dll" "thirdparty\windows\SDL3_image\SDL3_image.dll") do (
    if not exist "%%~F" (
        echo [ERROR] Missing dependency: %%~F
        goto :fail
    )
)

for %%D in ("%OUT%" "%OUT%\scenes" "%OUT%\fonts" "%OUT%\sounds" "%OUT%\assets") do (
    if not exist "%%~D" mkdir "%%~D"
)

copy /Y "thirdparty\windows\SDL3\SDL3.dll" "%OUT%\" >nul || goto :fail
copy /Y "thirdparty\windows\SDL3_ttf\SDL3_ttf.dll" "%OUT%\" >nul || goto :fail
copy /Y "thirdparty\windows\SDL3_image\SDL3_image.dll" "%OUT%\" >nul || goto :fail

if not exist "engine\fonts\debug.ttf" (
    echo [ERROR] Missing engine\fonts\debug.ttf
    goto :fail
)
copy /Y "engine\fonts\debug.ttf" "%OUT%\fonts\" >nul || goto :fail
if exist "engine\sounds\*.wav" (
    copy /Y "engine\sounds\*.wav" "%OUT%\sounds\" >nul || goto :fail
)
if exist "engine\assets\" (
    xcopy "engine\assets\*" "%OUT%\assets\" /E /I /Y /Q >nul
    if errorlevel 1 goto :fail
)
if exist "engine\VERSION" (
    if not exist "%OUT%\engine" mkdir "%OUT%\engine"
    copy /Y "engine\VERSION" "%OUT%\engine\" >nul || goto :fail
)

echo [1/2] Building %SCENE%
"%CXX%" %CXXFLAGS% %SDL_INCLUDES% -shared ^
    engine\scenes\main_scene.cpp %ENGINE_SOURCES% ^
    -o "%SCENE%" %SDL_DLLS% %LDFLAGS%
if errorlevel 1 goto :fail

echo [2/2] Building %EXE%
"%CXX%" %CXXFLAGS% %SDL_INCLUDES% ^
    engine\cpp\main.cpp engine\cpp\game.cpp %ENGINE_SOURCES% ^
    -o "%EXE%" %SDL_DLLS% %LDFLAGS%
if errorlevel 1 goto :fail

echo.
echo [OK] Windows release build completed.
for %%F in ("%EXE%" "%SCENE%") do echo      %%~F : %%~zF bytes
echo      Run from %OUT% so SDL can find the packaged assets.
popd
exit /b 0

:fail
echo.
echo [ERROR] Build failed. See the first error above.
popd
exit /b 1
