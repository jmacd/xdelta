#!/usr/bin/env pwsh
# Script to verify release artifacts
# Usage: ./scripts/verify-release-artifacts.ps1 -ArtifactsDir ./release-files -Version 3.1.0

param(
    [Parameter(Mandatory=$true)]
    [string]$ArtifactsDir,

    [Parameter(Mandatory=$true)]
    [string]$Version,

    [Parameter(Mandatory=$false)]
    [string]$ReleaseUrl = ""
)

# Initialize success flag
$success = $true

# Function to check a file
function Test-ReleaseFile {
    param(
        [string]$Path,
        [string]$Description,
        [int]$MinSize = 1000
    )

    if (Test-Path $Path) {
        $fileSize = (Get-Item $Path).Length

        if ($fileSize -gt $MinSize) {
            Write-Host "✅ $Description verified: $Path ($fileSize bytes)" -ForegroundColor Green
            return $true
        } else {
            Write-Host "❌ $Description is too small: $Path ($fileSize bytes)" -ForegroundColor Red
            return $false
        }
    } else {
        Write-Host "❌ $Description not found: $Path" -ForegroundColor Red
        return $false
    }
}

# Function to verify a ZIP file
function Test-ZipFile {
    param(
        [string]$Path,
        [string]$Description,
        [string[]]$RequiredFiles = @()
    )

    if (-not (Test-ReleaseFile -Path $Path -Description $Description)) {
        return $false
    }

    # Check ZIP contents
    try {
        $tempDir = Join-Path $env:TEMP "xdelta-verify-$(Get-Random)"
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

        Write-Host "Extracting $Description to verify contents..."
        Expand-Archive -Path $Path -DestinationPath $tempDir -Force

        $missingFiles = @()

        # Check for required files
        foreach ($requiredFile in $RequiredFiles) {
            $requiredPath = Join-Path $tempDir $requiredFile
            if (-not (Test-Path $requiredPath) -or (Get-Item $requiredPath).Length -eq 0) {
                $missingFiles += $requiredFile
            }
        }

        # Clean up
        Remove-Item -Path $tempDir -Recurse -Force

        if ($missingFiles.Count -eq 0) {
            Write-Host "✅ All required files present in $Description" -ForegroundColor Green
            return $true
        } else {
            Write-Host "❌ Missing required files in $Description: $($missingFiles -join ', ')" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "❌ Failed to verify $Description contents: $_" -ForegroundColor Red
        return $false
    }
}

# If ReleaseUrl is provided, download and verify files
if ($ReleaseUrl) {
    Write-Host "Verifying release from URL: $ReleaseUrl"

    # Create temp directory for downloads
    $downloadDir = Join-Path $env:TEMP "xdelta-download-$(Get-Random)"
    New-Item -ItemType Directory -Path $downloadDir -Force | Out-Null

    # Define files to download
    $filesToDownload = @(
        @{Name = "Windows x64"; File = "xdelta3-windows-x64.zip"},
        @{Name = "Windows x86"; File = "xdelta3-windows-x86.zip"},
        @{Name = "Linux"; File = "xdelta3-linux.tar.gz"},
        @{Name = "vcpkg binaries"; File = "xdelta-$Version-windows.zip"},
        @{Name = "SHA512 hash"; File = "xdelta-$Version-windows.zip.sha512"}
    )

    # Download files
    foreach ($file in $filesToDownload) {
        $url = "$ReleaseUrl/$($file.File)"
        $outputPath = Join-Path $downloadDir $file.File

        try {
            Write-Host "Downloading $($file.Name) from $url..."
            Invoke-WebRequest -Uri $url -OutFile $outputPath
            Write-Host "✅ Downloaded $($file.Name)" -ForegroundColor Green
        } catch {
            Write-Host "❌ Failed to download $($file.Name): $_" -ForegroundColor Red
            $success = $false
        }
    }

    # Set ArtifactsDir to the download directory
    $ArtifactsDir = $downloadDir
}

# Verify Windows x64 artifact
$winX64Path = Join-Path $ArtifactsDir "xdelta3-windows-x64.zip"
$winX64Success = Test-ZipFile -Path $winX64Path -Description "Windows x64 artifact" -RequiredFiles @("xdelta3.exe", "README.txt")
$success = $success -and $winX64Success

# Verify Windows x86 artifact
$winX86Path = Join-Path $ArtifactsDir "xdelta3-windows-x86.zip"
$winX86Success = Test-ZipFile -Path $winX86Path -Description "Windows x86 artifact" -RequiredFiles @("xdelta3.exe", "README.txt")
$success = $success -and $winX86Success

# Verify Linux artifact
$linuxPath = Join-Path $ArtifactsDir "xdelta3-linux.tar.gz"
$linuxSuccess = Test-ReleaseFile -Path $linuxPath -Description "Linux artifact"
$success = $success -and $linuxSuccess

# Verify vcpkg binaries
$vcpkgPath = Join-Path $ArtifactsDir "xdelta-$Version-windows.zip"
$vcpkgSuccess = Test-ZipFile -Path $vcpkgPath -Description "vcpkg binaries" -RequiredFiles @(
    "$Version/x64-windows/bin/xdelta3.exe",
    "$Version/x64-windows/lib/xdelta.lib",
    "$Version/x86-windows/bin/xdelta3.exe",
    "$Version/x86-windows/lib/xdelta.lib",
    "$Version/README.md"
)
$success = $success -and $vcpkgSuccess

# Verify SHA512 file
$sha512Path = Join-Path $ArtifactsDir "xdelta-$Version-windows.zip.sha512"
$sha512Success = Test-ReleaseFile -Path $sha512Path -Description "SHA512 hash file" -MinSize 10
$success = $success -and $sha512Success

# If SHA512 file exists, verify it matches the actual file
if ($sha512Success -and $vcpkgSuccess) {
    $actualHash = (Get-FileHash -Path $vcpkgPath -Algorithm SHA512).Hash.ToLower()
    $storedHash = Get-Content $sha512Path -Raw

    if ($storedHash -eq $actualHash) {
        Write-Host "✅ SHA512 hash verified for vcpkg binaries" -ForegroundColor Green
    } else {
        Write-Host "❌ SHA512 hash mismatch for vcpkg binaries" -ForegroundColor Red
        $success = $false
    }
}

# Clean up downloaded files if using ReleaseUrl
if ($ReleaseUrl -and (Test-Path $downloadDir)) {
    Remove-Item -Path $downloadDir -Recurse -Force
    Write-Host "Cleaned up downloaded files"
}

# Summary
Write-Host "`n=== Verification Summary ===" -ForegroundColor Cyan
if ($success) {
    Write-Host "✅ All checks passed!" -ForegroundColor Green
} else {
    Write-Host "❌ Verification failed" -ForegroundColor Red
}

# Return success status
return $success
