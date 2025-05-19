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

# Configure for x64 with liblzma support
Write-Host "Setting up vcpkg for x64..." -ForegroundColor Cyan

# Check if vcpkg is available
$vcpkgPath = "C:\vcpkg"
if (-not (Test-Path $vcpkgPath)) {
    $vcpkgPath = $env:VCPKG_ROOT
    if (-not $vcpkgPath -or -not (Test-Path $vcpkgPath)) {
        Write-Host "vcpkg not found. Trying to find it in the current directory..." -ForegroundColor Yellow
        $vcpkgPath = Join-Path (Get-Location) "vcpkg"
        if (-not (Test-Path $vcpkgPath)) {
            Write-Host "vcpkg not found. Will build without liblzma support." -ForegroundColor Yellow
            $enableLzma = "OFF"
            $vcpkgToolchain = ""
        } else {
            $enableLzma = "ON"
            $vcpkgToolchain = "-DCMAKE_TOOLCHAIN_FILE=`"$vcpkgPath\scripts\buildsystems\vcpkg.cmake`""
        }
    } else {
        $enableLzma = "ON"
        $vcpkgToolchain = "-DCMAKE_TOOLCHAIN_FILE=`"$vcpkgPath\scripts\buildsystems\vcpkg.cmake`""
    }
} else {
    $enableLzma = "ON"
    $vcpkgToolchain = "-DCMAKE_TOOLCHAIN_FILE=`"$vcpkgPath\scripts\buildsystems\vcpkg.cmake`""
}

# Configure for x64
Write-Host "Configuring for x64 with XDELTA_ENABLE_LZMA=$enableLzma..." -ForegroundColor Cyan
if ($enableLzma -eq "ON") {
    cmake -B build-x64 -S . -A x64 -DXDELTA_ENABLE_LZMA=ON $vcpkgToolchain -DVCPKG_TARGET_TRIPLET=x64-windows
} else {
    cmake -B build-x64 -S . -A x64 -DXDELTA_ENABLE_LZMA=OFF
}

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

# Copy liblzma library files if available
if ($enableLzma -eq "ON") {
    Write-Host "Copying liblzma library files for x64..." -ForegroundColor Cyan

    # Check for vcpkg installed libraries
    $vcpkgLibPath = Join-Path $vcpkgPath "installed\x64-windows\lib"
    $vcpkgBinPath = Join-Path $vcpkgPath "installed\x64-windows\bin"
    $vcpkgDebugLibPath = Join-Path $vcpkgPath "installed\x64-windows\debug\lib"
    $vcpkgDebugBinPath = Join-Path $vcpkgPath "installed\x64-windows\debug\bin"

    # Check for liblzma.lib in release
    if (Test-Path (Join-Path $vcpkgLibPath "liblzma.lib")) {
        Write-Host "Found liblzma.lib in $vcpkgLibPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgLibPath "liblzma.lib") -Destination $x64LibDir -Force
    } else {
        Write-Host "liblzma.lib not found in $vcpkgLibPath" -ForegroundColor Yellow
    }

    # Check for liblzma.dll in release
    if (Test-Path (Join-Path $vcpkgBinPath "liblzma.dll")) {
        Write-Host "Found liblzma.dll in $vcpkgBinPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgBinPath "liblzma.dll") -Destination $x64BinDir -Force
    } else {
        Write-Host "liblzma.dll not found in $vcpkgBinPath" -ForegroundColor Yellow
    }

    # Check for liblzma.lib in debug
    if (Test-Path (Join-Path $vcpkgDebugLibPath "liblzma.lib")) {
        Write-Host "Found debug liblzma.lib in $vcpkgDebugLibPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgDebugLibPath "liblzma.lib") -Destination (Join-Path $x64LibDir "liblzmad.lib") -Force
    } else {
        Write-Host "Debug liblzma.lib not found in $vcpkgDebugLibPath" -ForegroundColor Yellow
    }

    # Check for liblzma.dll in debug
    if (Test-Path (Join-Path $vcpkgDebugBinPath "liblzma.dll")) {
        Write-Host "Found debug liblzma.dll in $vcpkgDebugBinPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgDebugBinPath "liblzma.dll") -Destination (Join-Path $x64BinDir "liblzmad.dll") -Force
    } else {
        Write-Host "Debug liblzma.dll not found in $vcpkgDebugBinPath" -ForegroundColor Yellow
    }

    # Check for build-x64 vcpkg_installed directory
    $buildVcpkgPath = Join-Path (Get-Location) "build-x64\vcpkg_installed\x64-windows"
    if (Test-Path $buildVcpkgPath) {
        Write-Host "Found build-x64 vcpkg_installed directory" -ForegroundColor Green

        # Check for liblzma.lib in release
        $buildLibPath = Join-Path $buildVcpkgPath "lib"
        if (Test-Path (Join-Path $buildLibPath "liblzma.lib")) {
            Write-Host "Found liblzma.lib in $buildLibPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildLibPath "liblzma.lib") -Destination $x64LibDir -Force
        }

        # Check for liblzma.dll in release
        $buildBinPath = Join-Path $buildVcpkgPath "bin"
        if (Test-Path (Join-Path $buildBinPath "liblzma.dll")) {
            Write-Host "Found liblzma.dll in $buildBinPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildBinPath "liblzma.dll") -Destination $x64BinDir -Force
        }

        # Check for liblzma.lib in debug
        $buildDebugLibPath = Join-Path $buildVcpkgPath "debug\lib"
        if (Test-Path (Join-Path $buildDebugLibPath "liblzma.lib")) {
            Write-Host "Found debug liblzma.lib in $buildDebugLibPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildDebugLibPath "liblzma.lib") -Destination (Join-Path $x64LibDir "liblzmad.lib") -Force
        }

        # Check for liblzma.dll in debug
        $buildDebugBinPath = Join-Path $buildVcpkgPath "debug\bin"
        if (Test-Path (Join-Path $buildDebugBinPath "liblzma.dll")) {
            Write-Host "Found debug liblzma.dll in $buildDebugBinPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildDebugBinPath "liblzma.dll") -Destination (Join-Path $x64BinDir "liblzmad.dll") -Force
        }
    }
}

# Configure and build for x86
Write-Host "Configuring and building for x86..." -ForegroundColor Cyan
if (-not (Test-Path "build-x86")) {
    New-Item -ItemType Directory -Path "build-x86" | Out-Null
}

# Configure for x86 with liblzma support
Write-Host "Configuring for x86 with XDELTA_ENABLE_LZMA=$enableLzma..." -ForegroundColor Cyan
if ($enableLzma -eq "ON") {
    cmake -B build-x86 -S . -A Win32 -DXDELTA_ENABLE_LZMA=ON $vcpkgToolchain -DVCPKG_TARGET_TRIPLET=x86-windows
} else {
    cmake -B build-x86 -S . -A Win32 -DXDELTA_ENABLE_LZMA=OFF
}

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

# Copy liblzma library files if available
if ($enableLzma -eq "ON") {
    Write-Host "Copying liblzma library files for x86..." -ForegroundColor Cyan

    # Check for vcpkg installed libraries
    $vcpkgLibPath = Join-Path $vcpkgPath "installed\x86-windows\lib"
    $vcpkgBinPath = Join-Path $vcpkgPath "installed\x86-windows\bin"
    $vcpkgDebugLibPath = Join-Path $vcpkgPath "installed\x86-windows\debug\lib"
    $vcpkgDebugBinPath = Join-Path $vcpkgPath "installed\x86-windows\debug\bin"

    # Check for liblzma.lib in release
    if (Test-Path (Join-Path $vcpkgLibPath "liblzma.lib")) {
        Write-Host "Found liblzma.lib in $vcpkgLibPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgLibPath "liblzma.lib") -Destination $x86LibDir -Force
    } else {
        Write-Host "liblzma.lib not found in $vcpkgLibPath" -ForegroundColor Yellow
    }

    # Check for liblzma.dll in release
    if (Test-Path (Join-Path $vcpkgBinPath "liblzma.dll")) {
        Write-Host "Found liblzma.dll in $vcpkgBinPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgBinPath "liblzma.dll") -Destination $x86BinDir -Force
    } else {
        Write-Host "liblzma.dll not found in $vcpkgBinPath" -ForegroundColor Yellow
    }

    # Check for liblzma.lib in debug
    if (Test-Path (Join-Path $vcpkgDebugLibPath "liblzma.lib")) {
        Write-Host "Found debug liblzma.lib in $vcpkgDebugLibPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgDebugLibPath "liblzma.lib") -Destination (Join-Path $x86LibDir "liblzmad.lib") -Force
    } else {
        Write-Host "Debug liblzma.lib not found in $vcpkgDebugLibPath" -ForegroundColor Yellow
    }

    # Check for liblzma.dll in debug
    if (Test-Path (Join-Path $vcpkgDebugBinPath "liblzma.dll")) {
        Write-Host "Found debug liblzma.dll in $vcpkgDebugBinPath" -ForegroundColor Green
        Copy-Item (Join-Path $vcpkgDebugBinPath "liblzma.dll") -Destination (Join-Path $x86BinDir "liblzmad.dll") -Force
    } else {
        Write-Host "Debug liblzma.dll not found in $vcpkgDebugBinPath" -ForegroundColor Yellow
    }

    # Check for build-x86 vcpkg_installed directory
    $buildVcpkgPath = Join-Path (Get-Location) "build-x86\vcpkg_installed\x86-windows"
    if (Test-Path $buildVcpkgPath) {
        Write-Host "Found build-x86 vcpkg_installed directory" -ForegroundColor Green

        # Check for liblzma.lib in release
        $buildLibPath = Join-Path $buildVcpkgPath "lib"
        if (Test-Path (Join-Path $buildLibPath "liblzma.lib")) {
            Write-Host "Found liblzma.lib in $buildLibPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildLibPath "liblzma.lib") -Destination $x86LibDir -Force
        }

        # Check for liblzma.dll in release
        $buildBinPath = Join-Path $buildVcpkgPath "bin"
        if (Test-Path (Join-Path $buildBinPath "liblzma.dll")) {
            Write-Host "Found liblzma.dll in $buildBinPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildBinPath "liblzma.dll") -Destination $x86BinDir -Force
        }

        # Check for liblzma.lib in debug
        $buildDebugLibPath = Join-Path $buildVcpkgPath "debug\lib"
        if (Test-Path (Join-Path $buildDebugLibPath "liblzma.lib")) {
            Write-Host "Found debug liblzma.lib in $buildDebugLibPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildDebugLibPath "liblzma.lib") -Destination (Join-Path $x86LibDir "liblzmad.lib") -Force
        }

        # Check for liblzma.dll in debug
        $buildDebugBinPath = Join-Path $buildVcpkgPath "debug\bin"
        if (Test-Path (Join-Path $buildDebugBinPath "liblzma.dll")) {
            Write-Host "Found debug liblzma.dll in $buildDebugBinPath" -ForegroundColor Green
            Copy-Item (Join-Path $buildDebugBinPath "liblzma.dll") -Destination (Join-Path $x86BinDir "liblzmad.dll") -Force
        }
    }
}

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
    $(if ($enableLzma -eq "ON") { "    - liblzma.dll - LZMA library (if available)" })
    $(if ($enableLzma -eq "ON") { "    - liblzmad.dll - LZMA library debug build (if available)" })
  - lib/
    - xdelta.lib - Release build
    - xdeltad.lib - Debug build
    $(if ($enableLzma -eq "ON") { "    - liblzma.lib - LZMA library (if available)" })
    $(if ($enableLzma -eq "ON") { "    - liblzmad.lib - LZMA library debug build (if available)" })
  - include/xdelta3/
    - xdelta3.h
    - xdelta3.c

- x86-windows/
  - bin/
    - xdelta3.exe - Release build
    - xdelta3d.exe - Debug build
    $(if ($enableLzma -eq "ON") { "    - liblzma.dll - LZMA library (if available)" })
    $(if ($enableLzma -eq "ON") { "    - liblzmad.dll - LZMA library debug build (if available)" })
  - lib/
    - xdelta.lib - Release build
    - xdeltad.lib - Debug build
    $(if ($enableLzma -eq "ON") { "    - liblzma.lib - LZMA library (if available)" })
    $(if ($enableLzma -eq "ON") { "    - liblzmad.lib - LZMA library debug build (if available)" })
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

$(if ($enableLzma -eq "ON") { "If LZMA support is enabled, you also need to link against liblzma.lib." })

## Dependencies

$(if ($enableLzma -eq "ON") { "- liblzma: Used for LZMA compression support" } else { "- None" })

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

    # Basic verification of required files
    if ((Test-Path $extractedExePath) -and (Get-Item $extractedExePath).Length -gt 0 -and
        (Test-Path $extractedLibPath) -and (Get-Item $extractedLibPath).Length -gt 0) {
        Write-Host "Basic ZIP file contents verified: OK" -ForegroundColor Green

        # Additional verification for liblzma if enabled
        if ($enableLzma -eq "ON") {
            Write-Host "Verifying liblzma files in ZIP..." -ForegroundColor Cyan
            $extractedLzmaLibPath = Join-Path $tempExtractDir "$version\x64-windows\lib\liblzma.lib"
            $extractedLzmaDllPath = Join-Path $tempExtractDir "$version\x64-windows\bin\liblzma.dll"

            # Check if at least one of the liblzma files exists
            $lzmaFilesFound = $false

            if (Test-Path $extractedLzmaLibPath) {
                Write-Host "Found liblzma.lib in ZIP: $(if (Test-Path $extractedLzmaLibPath) { (Get-Item $extractedLzmaLibPath).Length } else { 'Not found' }) bytes" -ForegroundColor Green
                $lzmaFilesFound = $true
            }

            if (Test-Path $extractedLzmaDllPath) {
                Write-Host "Found liblzma.dll in ZIP: $(if (Test-Path $extractedLzmaDllPath) { (Get-Item $extractedLzmaDllPath).Length } else { 'Not found' }) bytes" -ForegroundColor Green
                $lzmaFilesFound = $true
            }

            if (-not $lzmaFilesFound) {
                Write-Host "WARNING: No liblzma files found in ZIP despite LZMA support being enabled" -ForegroundColor Yellow
                Write-Host "This may be normal if vcpkg was not properly set up or liblzma was not found during build" -ForegroundColor Yellow
                # We don't exit with error here, as the basic files are still present
            }
        }

        Write-Host "ZIP file contents verification complete" -ForegroundColor Green
    } else {
        Write-Host "ERROR: ZIP file contains missing or empty required files" -ForegroundColor Red
        Write-Host "xdelta3.exe: $(if (Test-Path $extractedExePath) { (Get-Item $extractedExePath).Length } else { 'Not found' }) bytes" -ForegroundColor Red
        Write-Host "xdelta.lib: $(if (Test-Path $extractedLibPath) { (Get-Item $extractedLibPath).Length } else { 'Not found' }) bytes" -ForegroundColor Red
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
