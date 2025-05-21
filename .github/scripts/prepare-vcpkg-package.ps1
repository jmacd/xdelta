#!/usr/bin/env pwsh
# Script to prepare vcpkg package structure from build artifacts
# Usage: ./prepare-vcpkg-package.ps1 -Version "3.1.0" -ArtifactsDir "downloaded-artifacts" -OutputDir "xdelta-package"

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,

    [Parameter(Mandatory=$true)]
    [string]$ArtifactsDir,

    [Parameter(Mandatory=$false)]
    [string]$OutputDir = "xdelta-$Version-windows"
)

# Ensure output directory exists
if (Test-Path $OutputDir) {
    Write-Host "Cleaning existing output directory: $OutputDir"
    Remove-Item -Path $OutputDir -Recurse -Force
}

Write-Host "Creating vcpkg package structure for xdelta version $Version"

# Create directory structure
$x64Dir = "$OutputDir/$Version/x64-windows"
$x86Dir = "$OutputDir/$Version/x86-windows"

Write-Host "Creating directory structure..."
New-Item -ItemType Directory -Path "$x64Dir/bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$x64Dir/lib" -Force | Out-Null
New-Item -ItemType Directory -Path "$x64Dir/include/xdelta3" -Force | Out-Null
New-Item -ItemType Directory -Path "$x86Dir/bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$x86Dir/lib" -Force | Out-Null
New-Item -ItemType Directory -Path "$x86Dir/include/xdelta3" -Force | Out-Null

# Copy x64 files
Write-Host "Copying x64 files..."
$x64ArtifactDir = "$ArtifactsDir/xdelta3-windows-x64"
if (Test-Path "$x64ArtifactDir/xdelta3.exe") {
    Copy-Item "$x64ArtifactDir/xdelta3.exe" -Destination "$x64Dir/bin/" -Force
    Write-Host "✅ Copied x64 executable"
} else {
    Write-Host "❌ x64 executable not found at: $x64ArtifactDir/xdelta3.exe"
    exit 1
}

# Copy x86 files
Write-Host "Copying x86 files..."
$x86ArtifactDir = "$ArtifactsDir/xdelta3-windows-x86"
if (Test-Path "$x86ArtifactDir/xdelta3.exe") {
    Copy-Item "$x86ArtifactDir/xdelta3.exe" -Destination "$x86Dir/bin/" -Force
    Write-Host "✅ Copied x86 executable"
} else {
    Write-Host "❌ x86 executable not found at: $x86ArtifactDir/xdelta3.exe"
    exit 1
}

# Copy header files (same for both architectures)
Write-Host "Copying header files..."
if (Test-Path "xdelta3/xdelta3.h") {
    Copy-Item "xdelta3/xdelta3.h" -Destination "$x64Dir/include/xdelta3/" -Force
    Copy-Item "xdelta3/xdelta3.h" -Destination "$x86Dir/include/xdelta3/" -Force
    Write-Host "✅ Copied header files"
} else {
    Write-Host "❌ Header file not found at: xdelta3/xdelta3.h"
    exit 1
}

# Create dummy lib files (for vcpkg compatibility)
Write-Host "Creating dummy lib files for vcpkg compatibility..."
New-Item -ItemType File -Path "$x64Dir/lib/xdelta.lib" -Force | Out-Null
New-Item -ItemType File -Path "$x64Dir/lib/xdeltad.lib" -Force | Out-Null
New-Item -ItemType File -Path "$x86Dir/lib/xdelta.lib" -Force | Out-Null
New-Item -ItemType File -Path "$x86Dir/lib/xdeltad.lib" -Force | Out-Null
Write-Host "✅ Created dummy lib files"

# Copy README
Write-Host "Copying README..."
if (Test-Path "README.md") {
    Copy-Item "README.md" -Destination "$OutputDir/$Version/" -Force
    Write-Host "✅ Copied README.md"
} else {
    Write-Host "⚠️ README.md not found, creating placeholder"
    Set-Content -Path "$OutputDir/$Version/README.md" -Value "# Xdelta $Version`n`nXdelta binary diff and differential compression tools.`n"
}

# Create usage instructions
Write-Host "Creating usage instructions..."
$usageContent = @"
# Using Xdelta with vcpkg

This package provides the xdelta3 command-line tool and header files for use with vcpkg.

## Command-line Usage

The xdelta3 executable is available at:
- \${VCPKG_INSTALLED_DIR}/\${VCPKG_TARGET_TRIPLET}/tools/xdelta/xdelta3.exe

## Including in Your Project

To use xdelta in your C/C++ project:

```cpp
#include <xdelta3/xdelta3.h>
```

For more information, see the [official documentation](https://github.com/jmacd/xdelta/blob/wiki/CommandLineSyntax.md).
"@
Set-Content -Path "$OutputDir/$Version/usage.md" -Value $usageContent
Write-Host "✅ Created usage instructions"

# Create zip file
Write-Host "Creating zip archive..."
$zipPath = "$OutputDir.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}
Compress-Archive -Path $OutputDir -DestinationPath $zipPath -Force
Write-Host "✅ Created zip archive: $zipPath"

# Calculate SHA512 hash
Write-Host "Calculating SHA512 hash..."
$sha512 = (Get-FileHash -Algorithm SHA512 $zipPath).Hash.ToLower()
Set-Content -Path "$zipPath.sha512" -Value $sha512
Write-Host "✅ Created SHA512 hash file: $zipPath.sha512"
Write-Host "SHA512: $sha512"

Write-Host "Package preparation complete!"
Write-Host "Output: $zipPath"
Write-Host "SHA512: $sha512"

# Return the SHA512 hash
return $sha512
