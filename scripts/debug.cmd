@echo off
REM Launch the Standalone under VS 2026's debugger.
REM Useful for stepping through audio callbacks without opening the IDE first.
REM Usage: scripts\debug.cmd

setlocal
set "ROOT=%~dp0.."
set "EXE=%ROOT%\vs2026-build\plugin\Picotado_artefacts\Debug\Standalone\Picotado.exe"
set "DEVENV=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.exe"

if not exist "%EXE%" (
  echo [debug] Standalone Debug not built. Try: scripts\build.cmd
  exit /b 1
)
if not exist "%DEVENV%" (
  echo [debug] Visual Studio 2026 not at expected path.
  exit /b 1
)

echo [debug] Attaching VS 2026 debugger to a fresh Picotado.exe ...
"%DEVENV%" /DebugExe "%EXE%"
endlocal
