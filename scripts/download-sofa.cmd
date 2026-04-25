@echo off
REM Download a free, well-known SOFA HRTF file (MIT KEMAR) into the
REM user-scope Picotado folder. The plugin's "Load SOFA…" file picker
REM defaults to that folder.
REM
REM Usage: scripts\download-sofa.cmd

setlocal
REM We use %APPDATA% (AppData\Roaming\Picotado\SOFA) — matches JUCE's
REM `userApplicationDataDirectory` so the plugin's file picker opens
REM here by default. We avoid ~/Documents because Windows Defender's
REM "Controlled Folder Access" often blocks writes there.
set "DEST=%APPDATA%\Picotado\SOFA"
if not exist "%DEST%" mkdir "%DEST%"
if not exist "%DEST%" (
  echo [download-sofa] Could not create %DEST%
  exit /b 1
)

set "URL_NORMAL=https://sofacoustics.org/data/database/mit/mit_kemar_normal_pinna.sofa"
set "URL_LARGE=https://sofacoustics.org/data/database/mit/mit_kemar_large_pinna.sofa"

echo [download-sofa] Saving to %DEST%
echo.
echo [1/2] mit_kemar_normal_pinna.sofa
powershell -ExecutionPolicy Bypass -NoProfile -Command ^
  "Invoke-WebRequest -Uri '%URL_NORMAL%' -OutFile '%DEST%\mit_kemar_normal_pinna.sofa' -UseBasicParsing"
if errorlevel 1 goto :err

echo [2/2] mit_kemar_large_pinna.sofa
powershell -ExecutionPolicy Bypass -NoProfile -Command ^
  "Invoke-WebRequest -Uri '%URL_LARGE%' -OutFile '%DEST%\mit_kemar_large_pinna.sofa' -UseBasicParsing"
if errorlevel 1 goto :err

echo.
echo [download-sofa] Done. In the plugin, click "Load SOFA..." and
echo                pick one of these files. The picker opens here
echo                by default.
endlocal
exit /b 0

:err
echo [download-sofa] Download failed. Check your internet connection or
echo                grab a SOFA file manually from sofaconventions.org
endlocal
exit /b 1
