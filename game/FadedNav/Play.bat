@echo off
rem Faded Nav tech test: builds (if needed) and runs the game in a window.
call "D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FadedNavEditor Win64 Development "-Project=%~dp0FadedNav.uproject" -WaitMutex -NoHotReload
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)
start "" "D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0FadedNav.uproject" -game -windowed -resx=1600 -resy=900
