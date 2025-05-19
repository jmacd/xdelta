#!/usr/bin/env pwsh
# Script to update vcpkg registry with new release information
# Usage: ./scripts/update-vcpkg-registry.ps1 -ReleaseTag v3.1.0 -BinaryZipUrl https://github.com/loonghao/xdelta/releases/download/v3.1.0/xdelta-3.1.0-windows.zip

param(
    [Parameter(Mandatory=$true)]
    [string]$ReleaseTag,

    [Parameter(Mandatory=$true)]
    [string]$BinaryZipUrl,

    [Parameter(Mandatory=$false)]
    [switch]$ValidateOnly = $false,

    [Parameter(Mandatory=$false)]
    [switch]$Force = $false
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

# Extract version from tag
$version = $ReleaseTag -replace '^v', ''
Write-Host "Updating vcpkg registry for version $version"

# Validate version format
if (-not ($version -match '^\d+\.\d+\.\d+$')) {
    Add-Warning "Version format '$version' does not match expected pattern (e.g., 3.1.0)"
}

# Validate URL format
if (-not ($BinaryZipUrl -match '^https://.*\.zip$')) {
    Add-Error "Binary ZIP URL '$BinaryZipUrl' does not appear to be a valid ZIP download URL"
}

# Temporary directory for downloads
try {
    # Use a fallback if TEMP environment variable is not set
    if ($env:TEMP) {
        $tempDir = Join-Path $env:TEMP "xdelta-vcpkg-update-$(Get-Random)"
    } else {
        # Fallback to current directory if TEMP is not set
        $tempDir = Join-Path (Get-Location) "temp-xdelta-vcpkg-update-$(Get-Random)"
        Write-Host "TEMP environment variable not set, using current directory: $tempDir"
    }
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    Write-Host "Created temporary directory: $tempDir"
} catch {
    Add-Error "Failed to create temporary directory: $_"
}

# Download the binary package to calculate SHA512
if ($success) {
    try {
        $zipPath = Join-Path $tempDir "xdelta-$version-windows.zip"
        Write-Host "Downloading binary package from $BinaryZipUrl"

        # Use more robust download with retry logic
        $maxRetries = 3
        $retryCount = 0
        $downloadSuccess = $false

        while (-not $downloadSuccess -and $retryCount -lt $maxRetries) {
            try {
                Invoke-WebRequest -Uri $BinaryZipUrl -OutFile $zipPath -TimeoutSec 60

                if (Test-Path $zipPath) {
                    $fileSize = (Get-Item $zipPath).Length
                    if ($fileSize -gt 1000) {
                        Write-Host "✅ Successfully downloaded binary package: $zipPath ($fileSize bytes)" -ForegroundColor Green
                        $downloadSuccess = $true
                    } else {
                        Add-Warning "Downloaded file is suspiciously small: $fileSize bytes"
                        Remove-Item -Path $zipPath -Force
                        $retryCount++
                    }
                } else {
                    Add-Warning "Download appeared to succeed but file not found"
                    $retryCount++
                }
            } catch {
                Add-Warning "Download attempt $($retryCount + 1) failed: $_"
                $retryCount++
                Start-Sleep -Seconds 5
            }
        }

        if (-not $downloadSuccess) {
            Add-Error "Failed to download binary package after $maxRetries attempts"
        }
    } catch {
        Add-Error "Failed to download binary package: $_"
    }
}

# Calculate SHA512 hash
$sha512 = $null
if ($success -and (Test-Path $zipPath)) {
    try {
        $sha512 = (Get-FileHash -Path $zipPath -Algorithm SHA512).Hash.ToLower()
        Write-Host "SHA512: $sha512"

        # Verify SHA512 hash format
        if (-not ($sha512 -match '^[a-f0-9]{128}$')) {
            Add-Error "Generated SHA512 hash does not match expected format"
        }
    } catch {
        Add-Error "Failed to calculate SHA512 hash: $_"
    }
}

# Verify the ZIP file contents
if ($success -and (Test-Path $zipPath)) {
    try {
        Write-Host "Verifying ZIP file contents..."
        $extractDir = Join-Path $tempDir "extract"
        New-Item -ItemType Directory -Path $extractDir -Force | Out-Null

        # Extract the ZIP file
        Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force

        # Check for critical files
        $criticalFiles = @(
            "$version/x64-windows/bin/xdelta3.exe",
            "$version/x64-windows/lib/xdelta.lib",
            "$version/x86-windows/bin/xdelta3.exe",
            "$version/x86-windows/lib/xdelta.lib",
            "$version/README.md"
        )

        $missingFiles = @()
        foreach ($file in $criticalFiles) {
            $filePath = Join-Path $extractDir $file
            if (-not (Test-Path $filePath) -or (Get-Item $filePath).Length -eq 0) {
                $missingFiles += $file
            }
        }

        if ($missingFiles.Count -eq 0) {
            Write-Host "✅ ZIP file verification passed: All critical files present" -ForegroundColor Green
        } else {
            $msg = "ZIP file verification failed: Missing or empty files: $($missingFiles -join ', ')"
            if ($Force) {
                Add-Warning $msg
            } else {
                Add-Error $msg
            }
        }
    } catch {
        $msg = "Failed to verify ZIP file contents: $_"
        if ($Force) {
            Add-Warning $msg
        } else {
            Add-Error $msg
        }
    }
}

# If we're only validating, stop here
if ($ValidateOnly) {
    Write-Host "Validation only mode - not updating any files"

    # Clean up
    if (Test-Path $tempDir) {
        Remove-Item -Path $tempDir -Recurse -Force
    }

    # Return results
    if ($success) {
        Write-Host "✅ Validation successful" -ForegroundColor Green
        return $sha512
    } else {
        Write-Host "❌ Validation failed with $($errors.Count) errors" -ForegroundColor Red
        throw "Validation failed: $($errors -join '; ')"
    }
}

# Update portfile.cmake
if ($success -and $sha512) {
    try {
        $portfilePath = "vcpkg-registry/ports/xdelta/portfile.cmake"
        Write-Host "Updating $portfilePath"

        if (Test-Path $portfilePath) {
            $portfileContent = Get-Content $portfilePath -Raw
            $updatedContent = $portfileContent -replace 'SHA512 "to-be-filled-after-release"', "SHA512 `"$sha512`""
            $updatedContent = $updatedContent -replace 'SHA512 "将在发布后填写实际的哈希值"', "SHA512 `"$sha512`""
            $updatedContent = $updatedContent -replace 'SHA512 "[a-f0-9]+"', "SHA512 `"$sha512`""
            Set-Content -Path $portfilePath -Value $updatedContent
            Write-Host "✅ Updated SHA512 in portfile.cmake" -ForegroundColor Green
        } else {
            Add-Error "portfile.cmake not found at $portfilePath"
        }
    } catch {
        Add-Error "Failed to update portfile.cmake: $_"
    }
}

# Update vcpkg.json if needed
if ($success) {
    try {
        $vcpkgJsonPath = "vcpkg-registry/ports/xdelta/vcpkg.json"
        Write-Host "Updating $vcpkgJsonPath"

        if (Test-Path $vcpkgJsonPath) {
            $vcpkgJsonContent = Get-Content $vcpkgJsonPath -Raw
            $updatedVcpkgJson = $vcpkgJsonContent -replace '"version": "[0-9.]*"', "`"version`": `"$version`""
            Set-Content -Path $vcpkgJsonPath -Value $updatedVcpkgJson
            Write-Host "✅ Updated version in vcpkg.json" -ForegroundColor Green
        } else {
            Add-Error "vcpkg.json not found at $vcpkgJsonPath"
        }
    } catch {
        Add-Error "Failed to update vcpkg.json: $_"
    }
}

# Update versions/x-/xdelta.json
if ($success) {
    try {
        $versionFilePath = "vcpkg-registry/versions/x-/xdelta.json"
        Write-Host "Updating $versionFilePath"

        if (Test-Path $versionFilePath) {
            $versionFileContent = Get-Content $versionFilePath -Raw

            # Get the current commit hash to use as git-tree
            $gitTree = git rev-parse HEAD
            Write-Host "Using git-tree: $gitTree"

            $updatedVersionContent = $versionFileContent -replace '"git-tree": "to-be-filled-after-release"', "`"git-tree`": `"$gitTree`""
            $updatedVersionContent = $updatedVersionContent -replace '"git-tree": "placeholder"', "`"git-tree`": `"$gitTree`""
            $updatedVersionContent = $updatedVersionContent -replace '"git-tree": "[a-f0-9]+"', "`"git-tree`": `"$gitTree`""
            $updatedVersionContent = $updatedVersionContent -replace '"version": "[0-9.]*"', "`"version`": `"$version`""
            Set-Content -Path $versionFilePath -Value $updatedVersionContent
            Write-Host "✅ Updated git-tree and version in xdelta.json" -ForegroundColor Green
        } else {
            Add-Error "xdelta.json not found at $versionFilePath"
        }
    } catch {
        Add-Error "Failed to update xdelta.json: $_"
    }
}

# Update baseline.json
if ($success) {
    try {
        $baselineJsonPath = "vcpkg-registry/versions/baseline.json"
        Write-Host "Updating $baselineJsonPath"

        if (Test-Path $baselineJsonPath) {
            $baselineJsonContent = Get-Content $baselineJsonPath -Raw
            $updatedBaselineJson = $baselineJsonContent -replace '"baseline": "[0-9.]*"', "`"baseline`": `"$version`""
            Set-Content -Path $baselineJsonPath -Value $updatedBaselineJson
            Write-Host "✅ Updated baseline in baseline.json" -ForegroundColor Green
        } else {
            Add-Error "baseline.json not found at $baselineJsonPath"
        }
    } catch {
        Add-Error "Failed to update baseline.json: $_"
    }
}

# Clean up
if (Test-Path $tempDir) {
    Remove-Item -Path $tempDir -Recurse -Force
    Write-Host "Cleaned up temporary directory"
}

# Output summary
Write-Host "`n=== Update Summary ===" -ForegroundColor Cyan
if ($success) {
    Write-Host "✅ vcpkg registry update completed successfully" -ForegroundColor Green
} else {
    Write-Host "❌ vcpkg registry update completed with $($errors.Count) errors:" -ForegroundColor Red
    $errors | ForEach-Object { Write-Host "- $_" -ForegroundColor Red }
}

if ($warnings.Count -gt 0) {
    Write-Host "⚠️ $($warnings.Count) warnings were found:" -ForegroundColor Yellow
    $warnings | ForEach-Object { Write-Host "- $_" -ForegroundColor Yellow }
}

# Return the SHA512 hash for use in GitHub Actions
if ($success) {
    return $sha512
} else {
    throw "Update failed: $($errors -join '; ')"
}
