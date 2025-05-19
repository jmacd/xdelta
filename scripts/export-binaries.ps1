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

# Configure and build for x86
Write-Host "Configuring and building for x86..." -ForegroundColor Cyan
if (-not (Test-Path "build-x86")) {
    New-Item -ItemType Directory -Path "build-x86" | Out-Null
}

# Configure for x86
cmake -B build-x86 -S . -A Win32 -DXDELTA_ENABLE_LZMA=OFF
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to configure for x86" -ForegroundColor Red
    exit 1
}

# Build Debug configuration for x86
Write-Host "Building Debug configuration for x86..."
cmake --build build-x86 --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to build Debug configuration for x86" -ForegroundColor Red
    exit 1
}

# Build Release configuration for x86
Write-Host "Building Release configuration for x86..."
cmake --build build-x86 --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to build Release configuration for x86" -ForegroundColor Red
    exit 1
}

# Copy x86 binaries
$x86BinDir = Join-Path $versionDir "x86-windows\bin"
$x86LibDir = Join-Path $versionDir "x86-windows\lib"
$x86IncludeDir = Join-Path $versionDir "x86-windows\include\xdelta3"

# Create include directory if it doesn't exist
if (-not (Test-Path $x86IncludeDir)) {
    New-Item -ItemType Directory -Path $x86IncludeDir -Force | Out-Null
}

# Copy Debug binaries for x86
Copy-Item "build-x86\Debug\xdelta3.exe" -Destination (Join-Path $x86BinDir "xdelta3d.exe") -Force
Copy-Item "build-x86\Debug\xdelta.lib" -Destination (Join-Path $x86LibDir "xdeltad.lib") -Force

# Copy Release binaries for x86
Copy-Item "build-x86\Release\xdelta3.exe" -Destination (Join-Path $x86BinDir "xdelta3.exe") -Force
Copy-Item "build-x86\Release\xdelta.lib" -Destination (Join-Path $x86LibDir "xdelta.lib") -Force

# Copy header files
Copy-Item "xdelta3\xdelta3.h" -Destination $x86IncludeDir -Force
Copy-Item "xdelta3\xdelta3.c" -Destination $x86IncludeDir -Force

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

- x86-windows/
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

# Verify binary files before creating ZIP
Write-Host "Verifying binary files..." -ForegroundColor Cyan

# Check x64 binaries
$x64ExePath = Join-Path $x64BinDir "xdelta3.exe"
$x64LibPath = Join-Path $x64LibDir "xdelta.lib"

if ((Test-Path $x64ExePath) -and (Get-Item $x64ExePath).Length -gt 0 -and
    (Test-Path $x64LibPath) -and (Get-Item $x64LibPath).Length -gt 0) {
    Write-Host "x64 binaries verified: OK" -ForegroundColor Green
} else {
    Write-Host "ERROR: x64 binaries are missing or empty" -ForegroundColor Red
    Write-Host "xdelta3.exe: $(if (Test-Path $x64ExePath) { (Get-Item $x64ExePath).Length } else { 'Not found' })" -ForegroundColor Red
    Write-Host "xdelta.lib: $(if (Test-Path $x64LibPath) { (Get-Item $x64LibPath).Length } else { 'Not found' })" -ForegroundColor Red
    exit 1
}

# Check x86 binaries
$x86ExePath = Join-Path $x86BinDir "xdelta3.exe"
$x86LibPath = Join-Path $x86LibDir "xdelta.lib"

if ((Test-Path $x86ExePath) -and (Get-Item $x86ExePath).Length -gt 0 -and
    (Test-Path $x86LibPath) -and (Get-Item $x86LibPath).Length -gt 0) {
    Write-Host "x86 binaries verified: OK" -ForegroundColor Green
} else {
    Write-Host "ERROR: x86 binaries are missing or empty" -ForegroundColor Red
    Write-Host "xdelta3.exe: $(if (Test-Path $x86ExePath) { (Get-Item $x86ExePath).Length } else { 'Not found' })" -ForegroundColor Red
    Write-Host "xdelta.lib: $(if (Test-Path $x86LibPath) { (Get-Item $x86LibPath).Length } else { 'Not found' })" -ForegroundColor Red
    exit 1
}

# Create a ZIP file
$zipPath = Join-Path $OutputDir "xdelta-$version-windows.zip"
Write-Host "Creating ZIP file: $zipPath" -ForegroundColor Cyan
Compress-Archive -Path $versionDir -DestinationPath $zipPath -Force

# Verify the ZIP file was created successfully
if (-not (Test-Path $zipPath)) {
    Write-Host "ERROR: Failed to create ZIP file" -ForegroundColor Red
    exit 1
}

# Calculate SHA512 hash for vcpkg
$sha512 = (Get-FileHash -Path $zipPath -Algorithm SHA512).Hash.ToLower()
Write-Host "SHA512: $sha512" -ForegroundColor Green

# Create a hash file for reference
$sha512 | Out-File -FilePath "$zipPath.sha512" -Encoding utf8 -NoNewline

# Verify the ZIP file contains valid binaries
Write-Host "Verifying ZIP file contents..." -ForegroundColor Cyan
$tempExtractDir = Join-Path $env:TEMP "xdelta-verify"
if (Test-Path $tempExtractDir) {
    Remove-Item -Path $tempExtractDir -Recurse -Force
}
New-Item -ItemType Directory -Path $tempExtractDir -Force | Out-Null

try {
    # Extract the ZIP file
    Expand-Archive -Path $zipPath -DestinationPath $tempExtractDir -Force

    # Check if the extracted files have content
    $extractedExePath = Join-Path $tempExtractDir "$version\x64-windows\bin\xdelta3.exe"
    $extractedLibPath = Join-Path $tempExtractDir "$version\x64-windows\lib\xdelta.lib"

    if ((Test-Path $extractedExePath) -and (Get-Item $extractedExePath).Length -gt 0 -and
        (Test-Path $extractedLibPath) -and (Get-Item $extractedLibPath).Length -gt 0) {
        Write-Host "ZIP file contents verified: OK" -ForegroundColor Green
    } else {
        Write-Host "ERROR: ZIP file contains missing or empty files" -ForegroundColor Red
        Write-Host "xdelta3.exe: $(if (Test-Path $extractedExePath) { (Get-Item $extractedExePath).Length } else { 'Not found' })" -ForegroundColor Red
        Write-Host "xdelta.lib: $(if (Test-Path $extractedLibPath) { (Get-Item $extractedLibPath).Length } else { 'Not found' })" -ForegroundColor Red
        exit 1
    }
} finally {
    # Clean up
    if (Test-Path $tempExtractDir) {
        Remove-Item -Path $tempExtractDir -Recurse -Force
    }
}

Write-Host "Export completed successfully!" -ForegroundColor Green
Write-Host "Binaries exported to: $zipPath"
Write-Host "Use this SHA512 hash in your vcpkg portfile: $sha512"
