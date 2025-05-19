#!/usr/bin/env pwsh
# Script to verify release artifacts
# Usage: ./scripts/verify-release-artifacts.ps1 -ArtifactsDir ./release-files -Version 3.1.0

param(
    [Parameter(Mandatory=$true)]
    [string]$ArtifactsDir,
    
    [Parameter(Mandatory=$true)]
    [string]$Version
)

# Initialize result object
$result = @{
    Success = $true
    Errors = @()
    Warnings = @()
    Details = @{}
}

# Function to add an error
function Add-Error {
    param([string]$Message)
    Write-Host "ERROR: $Message" -ForegroundColor Red
    $script:result.Errors += $Message
    $script:result.Success = $false
}

# Function to add a warning
function Add-Warning {
    param([string]$Message)
    Write-Host "WARNING: $Message" -ForegroundColor Yellow
    $script:result.Warnings += $Message
}

# Function to verify a file exists and has content
function Test-FileValid {
    param(
        [string]$Path,
        [string]$Description,
        [bool]$Critical = $true,
        [int]$MinSize = 1000  # Minimum size in bytes
    )
    
    $fileInfo = $null
    $result = @{
        Exists = $false
        Size = 0
        Valid = $false
    }
    
    if (Test-Path $Path) {
        $fileInfo = Get-Item $Path
        $result.Exists = $true
        $result.Size = $fileInfo.Length
        
        if ($fileInfo.Length -gt $MinSize) {
            $result.Valid = $true
            Write-Host "✅ $Description verified: $Path ($($fileInfo.Length) bytes)" -ForegroundColor Green
        } else {
            if ($Critical) {
                Add-Error "$Description is too small: $Path ($($fileInfo.Length) bytes)"
            } else {
                Add-Warning "$Description is suspiciously small: $Path ($($fileInfo.Length) bytes)"
            }
        }
    } else {
        if ($Critical) {
            Add-Error "$Description not found: $Path"
        } else {
            Add-Warning "$Description not found: $Path"
        }
    }
    
    return $result
}

# Function to verify an executable
function Test-Executable {
    param(
        [string]$Path,
        [string]$Description
    )
    
    $fileResult = Test-FileValid -Path $Path -Description $Description
    if (-not $fileResult.Valid) {
        return $fileResult
    }
    
    # Additional checks for executables
    try {
        Write-Host "Testing $Description functionality..."
        $output = & $Path --help 2>&1
        
        # Check if the output contains expected help text
        if ($output -match "xdelta3" -and ($output -match "usage" -or $output -match "options")) {
            Write-Host "✅ $Description functionality verified" -ForegroundColor Green
            $fileResult.Functional = $true
        } else {
            Add-Warning "$Description does not produce expected help output"
            $fileResult.Functional = $false
        }
    } catch {
        Add-Warning "$Description execution failed: $_"
        $fileResult.Functional = $false
    }
    
    return $fileResult
}

# Function to verify a ZIP file
function Test-ZipFile {
    param(
        [string]$Path,
        [string]$Description,
        [bool]$Critical = $true,
        [string[]]$RequiredFiles = @()
    )
    
    $fileResult = Test-FileValid -Path $Path -Description $Description -Critical $Critical
    if (-not $fileResult.Valid) {
        return $fileResult
    }
    
    # Additional checks for ZIP files
    try {
        $tempDir = Join-Path $env:TEMP "xdelta-verify-$(Get-Random)"
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
        
        Write-Host "Extracting $Description to verify contents..."
        Expand-Archive -Path $Path -DestinationPath $tempDir -Force
        
        $fileResult.Contents = @()
        $fileResult.MissingRequired = @()
        
        # Check for required files
        foreach ($requiredFile in $RequiredFiles) {
            $requiredPath = Join-Path $tempDir $requiredFile
            if (Test-Path $requiredPath) {
                $fileSize = (Get-Item $requiredPath).Length
                $fileResult.Contents += @{
                    Path = $requiredFile
                    Size = $fileSize
                    Exists = $true
                }
                
                if ($fileSize -lt 1000) {
                    Add-Warning "Required file in $Description is suspiciously small: $requiredFile ($fileSize bytes)"
                }
            } else {
                $fileResult.MissingRequired += $requiredFile
                if ($Critical) {
                    Add-Error "Required file missing in $Description: $requiredFile"
                } else {
                    Add-Warning "Required file missing in $Description: $requiredFile"
                }
            }
        }
        
        # List all files in the ZIP
        $allFiles = Get-ChildItem -Path $tempDir -Recurse -File
        Write-Host "Found $($allFiles.Count) files in $Description"
        
        # Clean up
        Remove-Item -Path $tempDir -Recurse -Force
        
        if ($fileResult.MissingRequired.Count -eq 0) {
            Write-Host "✅ All required files present in $Description" -ForegroundColor Green
        }
    } catch {
        Add-Error "Failed to verify $Description contents: $_"
        $fileResult.ExtractError = $_.ToString()
    }
    
    return $fileResult
}

# Verify Windows x64 artifact
$winX64Path = Join-Path $ArtifactsDir "xdelta3-windows-x64.zip"
$result.Details.WindowsX64 = Test-ZipFile -Path $winX64Path -Description "Windows x64 artifact" -RequiredFiles @("xdelta3.exe", "README.txt")

# Verify Windows x86 artifact
$winX86Path = Join-Path $ArtifactsDir "xdelta3-windows-x86.zip"
$result.Details.WindowsX86 = Test-ZipFile -Path $winX86Path -Description "Windows x86 artifact" -RequiredFiles @("xdelta3.exe", "README.txt")

# Verify Linux artifact
$linuxPath = Join-Path $ArtifactsDir "xdelta3-linux.tar.gz"
$result.Details.Linux = Test-FileValid -Path $linuxPath -Description "Linux artifact"

# Verify vcpkg binaries
$vcpkgPath = Join-Path $ArtifactsDir "xdelta-$Version-windows.zip"
$result.Details.VcpkgBinaries = Test-ZipFile -Path $vcpkgPath -Description "vcpkg binaries" -RequiredFiles @(
    "$Version/x64-windows/bin/xdelta3.exe",
    "$Version/x64-windows/lib/xdelta.lib",
    "$Version/x86-windows/bin/xdelta3.exe",
    "$Version/x86-windows/lib/xdelta.lib",
    "$Version/README.md"
)

# Verify SHA512 file
$sha512Path = Join-Path $ArtifactsDir "xdelta-$Version-windows.zip.sha512"
$result.Details.Sha512 = Test-FileValid -Path $sha512Path -Description "SHA512 hash file" -MinSize 10

# If SHA512 file exists, verify it matches the actual file
if ($result.Details.Sha512.Exists -and $result.Details.VcpkgBinaries.Exists) {
    $actualHash = (Get-FileHash -Path $vcpkgPath -Algorithm SHA512).Hash.ToLower()
    $storedHash = Get-Content $sha512Path -Raw
    
    if ($storedHash -eq $actualHash) {
        Write-Host "✅ SHA512 hash verified for vcpkg binaries" -ForegroundColor Green
        $result.Details.Sha512.Matches = $true
    } else {
        Add-Error "SHA512 hash mismatch for vcpkg binaries"
        $result.Details.Sha512.Matches = $false
        $result.Details.Sha512.Expected = $actualHash
        $result.Details.Sha512.Actual = $storedHash
    }
}

# Summary
Write-Host "`n=== Verification Summary ===" -ForegroundColor Cyan
if ($result.Success) {
    Write-Host "✅ All critical checks passed!" -ForegroundColor Green
} else {
    Write-Host "❌ Verification failed with $($result.Errors.Count) errors" -ForegroundColor Red
}

if ($result.Warnings.Count -gt 0) {
    Write-Host "⚠️ $($result.Warnings.Count) warnings were found" -ForegroundColor Yellow
}

# Return result object
return $result
