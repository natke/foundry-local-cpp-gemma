$ErrorActionPreference = "Stop"

$version = "2.0.1"
$ortGenAiVersion = "0.15.2"
$root = Split-Path -Parent $PSScriptRoot
$installDir = Join-Path $root ".foundry-local"
$header = Join-Path $installDir "include/foundry_local/foundry_local_cpp.h"

if (-not (Test-Path $header)) {
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
} else {
    Write-Host "Foundry Local $version is already installed in $installDir"
}

$includeDir = Join-Path $installDir "include"
New-Item -ItemType Directory -Force -Path $includeDir | Out-Null
foreach ($ortHeader in @("ort_genai.h", "ort_genai_c.h")) {
    $destination = Join-Path $includeDir $ortHeader
    if (-not (Test-Path $destination)) {
        $url = "https://raw.githubusercontent.com/microsoft/onnxruntime-genai/v$ortGenAiVersion/src/$ortHeader"
        Write-Host "Downloading $url"
        Invoke-WebRequest -Uri $url -OutFile $destination
    }
}
