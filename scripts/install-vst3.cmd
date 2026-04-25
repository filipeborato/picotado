@echo off
REM Copy the built VST3 into the user-scope VST3 path so DAWs scan it.
REM Usage: scripts\install-vst3.cmd            (Debug)
REM        scripts\install-vst3.cmd release    (Release)

setlocal
set "ROOT=%~dp0.."
set "CONFIG=Debug"
if /I "%~1"=="release" set "CONFIG=Release"
if /I "%~1"=="rel"     set "CONFIG=Release"

set "SRC=%ROOT%\vs2026-build\plugin\Picotado_artefacts\%CONFIG%\VST3\Picotado.vst3"
set "DST_USER=%LOCALAPPDATA%\Programs\Common\VST3"

if not exist "%SRC%" (
  echo [install-vst3] VST3 not built yet. Try: scripts\build.cmd %CONFIG%
  exit /b 1
)

if not exist "%DST_USER%" mkdir "%DST_USER%"
echo [install-vst3] Copying %SRC%
echo                       to %DST_USER%\Picotado.vst3
xcopy /E /I /Y /Q "%SRC%" "%DST_USER%\Picotado.vst3" >nul
if errorlevel 1 (
  echo [install-vst3] Copy failed.
  exit /b 1
)
echo [install-vst3] Done. Rescan plug-ins in your DAW.
endlocal
