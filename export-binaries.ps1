# Script to export xdelta binaries for distribution
# Usage: .\export-binaries.ps1 [OutputDir]
#
# This script builds and packages xdelta binaries for distribution via GitHub Releases
# and vcpkg. It creates a structured directory layout that follows vcpkg conventions.

param(
    [string]$OutputDir = ".\xdelta-binaries"
)

# Get version from CMakeLists.txt
$cmakeContent = Get-Content "CMakeLists.txt" -Raw
if ($cmakeContent -match 'VERSION\s+(\d+\.\d+\.\d+)') {
    $version = $matches[1]
    Write-Host "Detected version: $version" -ForegroundColor Cyan
} else {
    $version = "3.1.0"
    Write-Host "Could not detect version, using default: $version" -ForegroundColor Yellow
}

# Create output directory if it doesn't exist
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
    Write-Host "Created output directory: $OutputDir"
}

# Create version-specific subdirectory
$versionDir = Join-Path $OutputDir $version
if (-not (Test-Path $versionDir)) {
    New-Item -ItemType Directory -Path $versionDir | Out-Null
    Write-Host "Created version directory: $versionDir"
}

# Create architecture-specific subdirectories
$archDirs = @("x64-windows", "x86-windows")
foreach ($arch in $archDirs) {
    $archDir = Join-Path $versionDir $arch
    if (-not (Test-Path $archDir)) {
        New-Item -ItemType Directory -Path $archDir | Out-Null
        Write-Host "Created architecture directory: $archDir"
    }

    # Create bin, lib, include directories
    $binDir = Join-Path $archDir "bin"
    $libDir = Join-Path $archDir "lib"
    $includeDir = Join-Path $archDir "include"

    if (-not (Test-Path $binDir)) {
        New-Item -ItemType Directory -Path $binDir | Out-Null
    }
    if (-not (Test-Path $libDir)) {
        New-Item -ItemType Directory -Path $libDir | Out-Null
    }
    if (-not (Test-Path $includeDir)) {
        New-Item -ItemType Directory -Path $includeDir | Out-Null
    }
}

# Configure and build for x64
Write-Host "Configuring and building for x64..." -ForegroundColor Cyan
if (-not (Test-Path "build-x64")) {
    New-Item -ItemType Directory -Path "build-x64" | Out-Null
}

# Configure for x64
cmake -B build-x64 -S . -A x64 -DXDELTA_ENABLE_LZMA=OFF
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to configure for x64" -ForegroundColor Red
    exit 1
}

# Build Debug configuration for x64
Write-Host "Building Debug configuration for x64..."
cmake --build build-x64 --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to build Debug configuration for x64" -ForegroundColor Red
    exit 1
}

# Build Release configuration for x64
Write-Host "Building Release configuration for x64..."
cmake --build build-x64 --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to build Release configuration for x64" -ForegroundColor Red
    exit 1
}

# Copy x64 binaries
$x64BinDir = Join-Path $versionDir "x64-windows\bin"
$x64LibDir = Join-Path $versionDir "x64-windows\lib"
$x64IncludeDir = Join-Path $versionDir "x64-windows\include\xdelta3"

# Create include directory if it doesn't exist
if (-not (Test-Path $x64IncludeDir)) {
    New-Item -ItemType Directory -Path $x64IncludeDir -Force | Out-Null
}

# Copy Debug binaries for x64
Copy-Item "build-x64\Debug\xdelta3.exe" -Destination (Join-Path $x64BinDir "xdelta3d.exe") -Force
Copy-Item "build-x64\Debug\xdelta.lib" -Destination (Join-Path $x64LibDir "xdeltad.lib") -Force

# Copy Release binaries for x64
Copy-Item "build-x64\Release\xdelta3.exe" -Destination (Join-Path $x64BinDir "xdelta3.exe") -Force
Copy-Item "build-x64\Release\xdelta.lib" -Destination (Join-Path $x64LibDir "xdelta.lib") -Force

# Copy header files
Copy-Item "xdelta3\xdelta3.h" -Destination $x64IncludeDir -Force
Copy-Item "xdelta3\xdelta3.c" -Destination $x64IncludeDir -Force

# Create a README file
$readmePath = Join-Path $versionDir "README.md"
@"
# Xdelta Binary Distribution

Version: $version

## Contents

- x64-windows/
  - bin/
    - xdelta3.exe - Release build
    - xdelta3d.exe - Debug build
  - lib/
    - xdelta.lib - Release build
    - xdeltad.lib - Debug build
  - include/xdelta3/
    - xdelta3.h
    - xdelta3.c

## Usage

### Command Line

```
xdelta3.exe -h
```

### Library

Include xdelta3.h in your project and link against xdelta.lib.

## License

Licensed under the Apache License, Version 2.0
"@ | Out-File -FilePath $readmePath -Encoding utf8

# Create a ZIP file
$zipPath = Join-Path $OutputDir "xdelta-$version-windows.zip"
Write-Host "Creating ZIP file: $zipPath" -ForegroundColor Cyan
Compress-Archive -Path $versionDir -DestinationPath $zipPath -Force

# Calculate SHA512 hash for vcpkg
$sha512 = (Get-FileHash -Path $zipPath -Algorithm SHA512).Hash.ToLower()
Write-Host "SHA512: $sha512" -ForegroundColor Green

# Create a hash file for reference
$sha512 | Out-File -FilePath "$zipPath.sha512" -Encoding utf8 -NoNewline

Write-Host "Export completed successfully!" -ForegroundColor Green
Write-Host "Binaries exported to: $zipPath"
Write-Host "Use this SHA512 hash in your vcpkg portfile: $sha512"
