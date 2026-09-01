$ErrorActionPreference = "Stop"

$version = "2.0.1"
$root = Split-Path -Parent $PSScriptRoot
$installDir = Join-Path $root ".foundry-local"
$header = Join-Path $installDir "include/foundry_local/foundry_local_cpp.h"

if (Test-Path $header) {
    Write-Host "Foundry Local $version is already installed in $installDir"
    exit 0
}

switch ($env:PROCESSOR_ARCHITECTURE) {
    "AMD64" { $asset = "foundry-local-win-x64.zip" }
    "ARM64" { $asset = "foundry-local-win-arm64.zip" }
    default { throw "Unsupported Windows architecture: $env:PROCESSOR_ARCHITECTURE" }
}

$archive = Join-Path $env:TEMP $asset
$url = "https://github.com/microsoft/foundry-local/releases/download/v$version/$asset"

New-Item -ItemType Directory -Force -Path $installDir | Out-Null
Write-Host "Downloading $url"
Invoke-WebRequest -Uri $url -OutFile $archive
Expand-Archive -Path $archive -DestinationPath $installDir -Force
Remove-Item $archive
Write-Host "Installed Foundry Local $version in $installDir"
