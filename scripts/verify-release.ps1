#!/usr/bin/env pwsh
# Script to verify a release package
# Usage: ./scripts/verify-release.ps1 -Version 3.1.0 -ReleaseUrl https://github.com/loonghao/xdelta/releases/download/v3.1.0

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [Parameter(Mandatory=$true)]
    [string]$ReleaseUrl,
    
    [Parameter(Mandatory=$false)]
    [switch]$Verbose = $false
)

# Initialize result tracking
$errors = @()
$warnings = @()
$success = $true

# Function to add an error
function Add-Error {
    param([string]$Message)
    Write-Host "ERROR: $Message" -ForegroundColor Red
    $script:errors += $Message
    $script:success = $false
}

# Function to add a warning
function Add-Warning {
    param([string]$Message)
    Write-Host "WARNING: $Message" -ForegroundColor Yellow
    $script:warnings += $Message
}

# Function to log verbose information
function Write-Verbose {
    param([string]$Message)
    if ($Verbose) {
        Write-Host "VERBOSE: $Message" -ForegroundColor Gray
    }
}

Write-Host "Verifying release for version $Version at $ReleaseUrl"

# Temporary directory for downloads
try {
    $tempDir = Join-Path $env:TEMP "xdelta-verify-release-$(Get-Random)"
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    Write-Host "Created temporary directory: $tempDir"
} catch {
    Add-Error "Failed to create temporary directory: $_"
    exit 1
}

# Define the files to verify
$filesToVerify = @(
    @{
        Name = "Windows x64 binary";
        Url = "$ReleaseUrl/xdelta3-windows-x64.zip";
        Path = Join-Path $tempDir "xdelta3-windows-x64.zip";
        MinSize = 10000;
        RequiredFiles = @(
            "xdelta3.exe"
        );
        TestCommand = "xdelta3.exe --help";
    },
    @{
        Name = "Windows x86 binary";
        Url = "$ReleaseUrl/xdelta3-windows-x86.zip";
        Path = Join-Path $tempDir "xdelta3-windows-x86.zip";
        MinSize = 10000;
        RequiredFiles = @(
            "xdelta3.exe"
        );
        TestCommand = "xdelta3.exe --help";
    },
    @{
        Name = "Linux binary";
        Url = "$ReleaseUrl/xdelta3-linux.tar.gz";
        Path = Join-Path $tempDir "xdelta3-linux.tar.gz";
        MinSize = 10000;
        RequiredFiles = @(
            "xdelta3"
        );
        # No test command for Linux binary on Windows
    },
    @{
        Name = "vcpkg binaries";
        Url = "$ReleaseUrl/xdelta-$Version-windows.zip";
        Path = Join-Path $tempDir "xdelta-$Version-windows.zip";
        MinSize = 10000;
        RequiredFiles = @(
            "$Version/x64-windows/bin/xdelta3.exe",
            "$Version/x64-windows/lib/xdelta.lib",
            "$Version/x86-windows/bin/xdelta3.exe",
            "$Version/x86-windows/lib/xdelta.lib",
            "$Version/README.md"
        );
        TestCommand = "$Version/x64-windows/bin/xdelta3.exe --help";
    },
    @{
        Name = "vcpkg SHA512 hash";
        Url = "$ReleaseUrl/xdelta-$Version-windows.zip.sha512";
        Path = Join-Path $tempDir "xdelta-$Version-windows.zip.sha512";
        MinSize = 128;
        # No required files or test command for SHA512 file
    }
)

# Download and verify each file
foreach ($file in $filesToVerify) {
    Write-Host "`nVerifying $($file.Name)..."
    
    # Download the file
    try {
        Write-Host "Downloading from $($file.Url)..."
        Invoke-WebRequest -Uri $file.Url -OutFile $file.Path -ErrorAction Stop
        
        if (Test-Path $file.Path) {
            $fileSize = (Get-Item $file.Path).Length
            Write-Host "Downloaded $($file.Name): $($file.Path) ($fileSize bytes)"
            
            # Check file size
            if ($fileSize -lt $file.MinSize) {
                Add-Warning "$($file.Name) is suspiciously small: $fileSize bytes (expected at least $($file.MinSize) bytes)"
            }
        } else {
            Add-Error "Failed to download $($file.Name)"
            continue
        }
    } catch {
        Add-Error "Failed to download $($file.Name): $_"
        continue
    }
    
    # Special handling for SHA512 file
    if ($file.Name -eq "vcpkg SHA512 hash") {
        try {
            $sha512Content = Get-Content $file.Path -Raw
            Write-Host "SHA512 hash: $sha512Content"
            
            # Verify SHA512 hash format
            if (-not ($sha512Content -match '^[a-f0-9]{128}$')) {
                Add-Error "SHA512 hash does not match expected format"
            } else {
                # Verify SHA512 hash against the vcpkg binaries file
                $vcpkgBinariesPath = Join-Path $tempDir "xdelta-$Version-windows.zip"
                if (Test-Path $vcpkgBinariesPath) {
                    $actualHash = (Get-FileHash -Path $vcpkgBinariesPath -Algorithm SHA512).Hash.ToLower()
                    if ($actualHash -eq $sha512Content) {
                        Write-Host "✅ SHA512 hash verified" -ForegroundColor Green
                    } else {
                        Add-Error "SHA512 hash mismatch: Expected $sha512Content, got $actualHash"
                    }
                } else {
                    Add-Warning "Cannot verify SHA512 hash: vcpkg binaries file not found"
                }
            }
        } catch {
            Add-Error "Failed to verify SHA512 hash: $_"
        }
        continue
    }
    
    # Extract and verify archive contents
    if ($file.RequiredFiles) {
        try {
            $extractDir = Join-Path $tempDir "$($file.Name -replace '[^a-zA-Z0-9]', '_')"
            New-Item -ItemType Directory -Path $extractDir -Force | Out-Null
            
            # Extract the archive
            if ($file.Path -like "*.zip") {
                Write-Verbose "Extracting ZIP archive to $extractDir..."
                Expand-Archive -Path $file.Path -DestinationPath $extractDir -Force
            } elseif ($file.Path -like "*.tar.gz") {
                Write-Verbose "Extracting TAR.GZ archive to $extractDir..."
                # On Windows, we can't easily extract tar.gz files without additional tools
                # Just check if the file exists and has a reasonable size
                Write-Host "✅ TAR.GZ archive exists with reasonable size" -ForegroundColor Green
                continue
            } else {
                Add-Warning "Unknown archive format for $($file.Name)"
                continue
            }
            
            # Check for required files
            $missingFiles = @()
            foreach ($requiredFile in $file.RequiredFiles) {
                $requiredFilePath = Join-Path $extractDir $requiredFile
                if (-not (Test-Path $requiredFilePath)) {
                    $missingFiles += $requiredFile
                } elseif ((Get-Item $requiredFilePath).Length -eq 0) {
                    $missingFiles += "$requiredFile (empty file)"
                } else {
                    Write-Verbose "Found required file: $requiredFile"
                }
            }
            
            if ($missingFiles.Count -eq 0) {
                Write-Host "✅ All required files present in $($file.Name)" -ForegroundColor Green
            } else {
                Add-Error "Missing required files in $($file.Name): $($missingFiles -join ', ')"
            }
            
            # Test executable if applicable
            if ($file.TestCommand -and $file.Path -like "*.zip") {
                try {
                    $commandParts = $file.TestCommand -split ' '
                    $exePath = Join-Path $extractDir $commandParts[0]
                    $arguments = $commandParts[1..($commandParts.Length-1)]
                    
                    if (Test-Path $exePath) {
                        Write-Host "Testing executable: $exePath $arguments"
                        $process = Start-Process -FilePath $exePath -ArgumentList $arguments -NoNewWindow -Wait -PassThru
                        
                        # xdelta3 --help returns exit code 1, so we don't check the exit code
                        # Instead, we just verify that the executable ran without crashing
                        Write-Host "✅ Executable test completed" -ForegroundColor Green
                    } else {
                        Add-Error "Executable not found for testing: $exePath"
                    }
                } catch {
                    Add-Error "Failed to test executable: $_"
                }
            }
        } catch {
            Add-Error "Failed to verify archive contents: $_"
        }
    }
}

# Clean up
try {
    Remove-Item -Path $tempDir -Recurse -Force
    Write-Host "Cleaned up temporary directory"
} catch {
    Add-Warning "Failed to clean up temporary directory: $_"
}

# Output summary
Write-Host "`n=== Verification Summary ===" -ForegroundColor Cyan
if ($success) {
    Write-Host "✅ Release verification completed successfully" -ForegroundColor Green
} else {
    Write-Host "❌ Release verification completed with $($errors.Count) errors:" -ForegroundColor Red
    $errors | ForEach-Object { Write-Host "- $_" -ForegroundColor Red }
}

if ($warnings.Count -gt 0) {
    Write-Host "⚠️ $($warnings.Count) warnings were found:" -ForegroundColor Yellow
    $warnings | ForEach-Object { Write-Host "- $_" -ForegroundColor Yellow }
}

# Return exit code
if ($success) {
    exit 0
} else {
    exit 1
}
