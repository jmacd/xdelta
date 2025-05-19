#!/usr/bin/env pwsh
# Script to verify artifacts
# Usage: ./verify-artifacts.ps1 -ArtifactsDir <path_to_artifacts_dir> -Platform <platform> -Arch <arch>

param (
    [Parameter(Mandatory=$true)]
    [string]$ArtifactsDir,
    
    [Parameter(Mandatory=$true)]
    [ValidateSet("windows", "linux", "macos")]
    [string]$Platform,
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("x64", "x86")]
    [string]$Arch = "x64"
)

Write-Host "Verifying artifacts in: $ArtifactsDir"
Write-Host "Platform: $Platform"
Write-Host "Architecture: $Arch"

# Check if artifacts directory exists
if (-not (Test-Path $ArtifactsDir)) {
    Write-Host "❌ Artifacts directory not found: $ArtifactsDir"
    exit 1
}

# List all files in the artifacts directory
Write-Host "Files in artifacts directory:"
Get-ChildItem -Path $ArtifactsDir | ForEach-Object { Write-Host "  $($_.Name)" }

# Function to verify Windows artifacts
function Verify-WindowsArtifacts {
    param (
        [string]$Dir,
        [string]$Arch
    )
    
    # Check for xdelta3.exe
    if (-not (Test-Path "$Dir\xdelta3.exe")) {
        Write-Host "❌ xdelta3.exe not found in $Dir"
        return $false
    }
    
    # Check for README.txt
    if (-not (Test-Path "$Dir\README.txt")) {
        Write-Host "❌ README.txt not found in $Dir"
        return $false
    }
    
    # Check for liblzma.dll
    if (-not (Test-Path "$Dir\liblzma.dll")) {
        Write-Host "⚠️ liblzma.dll not found in $Dir (may be statically linked)"
    }
    
    Write-Host "✅ All required Windows $Arch files found"
    return $true
}

# Function to verify Linux artifacts
function Verify-LinuxArtifacts {
    param (
        [string]$Dir
    )
    
    # Check for xdelta3
    if (-not (Test-Path "$Dir\xdelta3")) {
        Write-Host "❌ xdelta3 not found in $Dir"
        return $false
    }
    
    # Check for README.txt
    if (-not (Test-Path "$Dir\README.txt")) {
        Write-Host "❌ README.txt not found in $Dir"
        return $false
    }
    
    Write-Host "✅ All required Linux files found"
    return $true
}

# Function to verify macOS artifacts
function Verify-MacOSArtifacts {
    param (
        [string]$Dir
    )
    
    # Check for README.txt (macOS build is disabled, so only README is required)
    if (-not (Test-Path "$Dir\README.txt")) {
        Write-Host "❌ README.txt not found in $Dir"
        return $false
    }
    
    Write-Host "✅ All required macOS files found (note: macOS build is disabled)"
    return $true
}

# Verify artifacts based on platform
$result = $false
switch ($Platform) {
    "windows" {
        $result = Verify-WindowsArtifacts -Dir $ArtifactsDir -Arch $Arch
    }
    "linux" {
        $result = Verify-LinuxArtifacts -Dir $ArtifactsDir
    }
    "macos" {
        $result = Verify-MacOSArtifacts -Dir $ArtifactsDir
    }
    default {
        Write-Host "❌ Unknown platform: $Platform"
        exit 1
    }
}

if ($result) {
    exit 0
} else {
    exit 1
}
