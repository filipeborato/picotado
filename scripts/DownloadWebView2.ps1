$packageSourceName = "nugetRepository"

$ErrorActionPreference = "SilentlyContinue"

# Ensure NuGet provider is bootstrapped (needed for fresh PowerShellGet installs)
Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Scope CurrentUser -Force | Out-Null

# JUCE 8.0.12 looks for the static loader inside this NuGet package version
$location = "https://www.nuget.org/api/v2"
Register-PackageSource -Provider NuGet -Name $packageSourceName -Location $location -Trusted -Force | Out-Null
Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.3485.44 -Source $packageSourceName -Force | Out-Null

if (-not (Test-Path "$env:USERPROFILE\AppData\Local\PackageManagement\NuGet\Packages\Microsoft.Web.WebView2.1.0.3485.44")) {
    Write-Error "WebView2 NuGet package was not installed. Check your internet connection or install manually."
    exit 1
}

