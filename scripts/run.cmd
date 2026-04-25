@echo off
REM Launch the standalone build for quick testing.
REM Usage: scripts\run.cmd            (Debug)
REM        scripts\run.cmd release    (Release)

setlocal
set "ROOT=%~dp0.."
set "CONFIG=Debug"
if /I "%~1"=="release" set "CONFIG=Release"
if /I "%~1"=="rel"     set "CONFIG=Release"

set "EXE=%ROOT%\vs2026-build\plugin\Picotado_artefacts\%CONFIG%\Standalone\Picotado.exe"
if not exist "%EXE%" (
  echo [run] Standalone not built yet. Try: scripts\build.cmd %CONFIG%
  exit /b 1
)

echo [run] Launching %EXE%
"%EXE%"
endlocal
