@echo off
REM Configure (if needed) and build Picotado.
REM Usage: scripts\build.cmd            (Debug)
REM        scripts\build.cmd release    (Release)
REM        scripts\build.cmd debug all  (build all targets, not just Picotado)

setlocal EnableDelayedExpansion

set "ROOT=%~dp0.."
pushd "%ROOT%" >nul

set "CMAKE=cmake"
where /q cmake
if errorlevel 1 set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"

set "CONFIG=Debug"
if /I "%~1"=="release" set "CONFIG=Release"
if /I "%~1"=="rel"     set "CONFIG=Release"

REM Picotado_All is JUCE's umbrella target that produces VST3 + Standalone +
REM the SharedCode lib in one go. "shared" builds only the SharedCode lib.
set "TARGET=Picotado_All"
if /I "%~2"=="shared" set "TARGET=Picotado"
if /I "%~2"=="vst3"   set "TARGET=Picotado_VST3"
if /I "%~2"=="standalone" set "TARGET=Picotado_Standalone"
if /I "%~2"=="all"    set "TARGET=ALL_BUILD"

if not exist "vs2026-build\CMakeCache.txt" (
  echo [build] Configuring vs2026 preset...
  "%CMAKE%" --preset vs2026
  if errorlevel 1 goto :err
)

echo [build] Building target %TARGET% (%CONFIG%) ...
cmd /c ""%CMAKE%" --build vs2026-build --config %CONFIG% --target %TARGET% --parallel -- /clp:ErrorsOnly;Summary /verbosity:minimal"
if errorlevel 1 goto :err

echo.
echo [build] Done. Artefacts at:
echo         vs2026-build\plugin\Picotado_artefacts\%CONFIG%\
popd >nul
endlocal & exit /b 0

:err
echo [build] FAILED.
popd >nul
endlocal & exit /b 1
