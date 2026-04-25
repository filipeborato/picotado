@echo off
REM One-shot release: build Release + install VST3 to user folder.
REM Usage: scripts\release.cmd

setlocal
set "ROOT=%~dp0.."

call "%~dp0build.cmd" release
if errorlevel 1 exit /b 1

call "%~dp0install-vst3.cmd" release
if errorlevel 1 exit /b 1

echo.
echo [release] DONE. Picotado.vst3 installed at:
echo     %LOCALAPPDATA%\Programs\Common\VST3\Picotado.vst3
echo Rescan plug-ins in your DAW.
endlocal
