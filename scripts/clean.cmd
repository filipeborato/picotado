@echo off
REM Wipe the CMake build tree (keeps libs/ cache so reconfiguring is fast).
REM Usage: scripts\clean.cmd
REM        scripts\clean.cmd all   (also wipe libs/, forcing a full re-download)

setlocal
set "ROOT=%~dp0.."
pushd "%ROOT%" >nul

if exist vs2026-build (
  echo [clean] Removing vs2026-build\
  rmdir /S /Q vs2026-build
)
if exist vs-build (
  echo [clean] Removing vs-build\
  rmdir /S /Q vs-build
)
if exist build (
  echo [clean] Removing build\
  rmdir /S /Q build
)
if exist release-build (
  echo [clean] Removing release-build\
  rmdir /S /Q release-build
)

if /I "%~1"=="all" (
  if exist libs (
    echo [clean] Removing libs\ ^(forces full re-download next configure^)
    rmdir /S /Q libs
  )
)

echo [clean] Done.
popd >nul
endlocal
