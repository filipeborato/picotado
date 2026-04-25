@echo off
REM One-time setup: install the WebView2 NuGet package that JUCE expects.
REM Re-running is harmless — it's a no-op once the package is present.

setlocal
set "ROOT=%~dp0.."

powershell -ExecutionPolicy Bypass -NoProfile -File "%ROOT%\scripts\DownloadWebView2.ps1"
if errorlevel 1 (
  echo [setup] WebView2 install failed.
  exit /b 1
)

echo [setup] WebView2 NuGet ready. You can now run scripts\build.cmd
endlocal
