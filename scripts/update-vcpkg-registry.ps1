#!/usr/bin/env pwsh
# Script to update vcpkg registry with new release information
# Usage: ./scripts/update-vcpkg-registry.ps1 -ReleaseTag v3.1.0 -BinaryZipUrl https://github.com/loonghao/xdelta/releases/download/v3.1.0/xdelta-3.1.0-windows.zip

param(
    [Parameter(Mandatory=$true)]
    [string]$ReleaseTag,

    [Parameter(Mandatory=$true)]
    [string]$BinaryZipUrl,

    [Parameter(Mandatory=$false)]
    [switch]$ValidateOnly = $false
)

# Extract version from tag
$version = $ReleaseTag -replace '^v', ''
Write-Host "Updating vcpkg registry for version $version"

# Temporary directory for downloads
$tempDir = Join-Path $env:TEMP "xdelta-vcpkg-update-$(Get-Random)"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
Write-Host "Created temporary directory: $tempDir"

# Download the binary package to calculate SHA512
try {
    $zipPath = Join-Path $tempDir "xdelta-$version-windows.zip"
    Write-Host "Downloading binary package from $BinaryZipUrl"

    Invoke-WebRequest -Uri $BinaryZipUrl -OutFile $zipPath

    if (Test-Path $zipPath) {
        $fileSize = (Get-Item $zipPath).Length
        if ($fileSize -gt 1000) {
            Write-Host "✅ Successfully downloaded binary package: $zipPath ($fileSize bytes)" -ForegroundColor Green
        } else {
            Write-Host "❌ Downloaded file is too small: $fileSize bytes" -ForegroundColor Red
            exit 1
        }
    } else {
        Write-Host "❌ Download failed - file not found" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "❌ Failed to download binary package: $_" -ForegroundColor Red
    exit 1
}

# Calculate SHA512 hash
try {
    $sha512 = (Get-FileHash -Path $zipPath -Algorithm SHA512).Hash.ToLower()
    Write-Host "SHA512: $sha512"
} catch {
    Write-Host "❌ Failed to calculate SHA512 hash: $_" -ForegroundColor Red
    exit 1
}

# Verify the ZIP file contents
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
        "$version/x86-windows/lib/xdelta.lib"
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
        Write-Host "❌ ZIP file verification failed: Missing files: $($missingFiles -join ', ')" -ForegroundColor Red
        if (-not $ValidateOnly) {
            Write-Host "Continuing despite missing files..." -ForegroundColor Yellow
        }
    }
} catch {
    Write-Host "❌ Failed to verify ZIP file contents: $_" -ForegroundColor Red
    if (-not $ValidateOnly) {
        Write-Host "Continuing despite verification failure..." -ForegroundColor Yellow
    }
}

# If we're only validating, stop here
if ($ValidateOnly) {
    Write-Host "Validation only mode - not updating any files"

    # Clean up
    Remove-Item -Path $tempDir -Recurse -Force

    return $sha512
}

# Update portfile.cmake
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
        Write-Host "❌ portfile.cmake not found at $portfilePath" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "❌ Failed to update portfile.cmake: $_" -ForegroundColor Red
    exit 1
}

# Update vcpkg.json
try {
    $vcpkgJsonPath = "vcpkg-registry/ports/xdelta/vcpkg.json"
    Write-Host "Updating $vcpkgJsonPath"

    if (Test-Path $vcpkgJsonPath) {
        $vcpkgJsonContent = Get-Content $vcpkgJsonPath -Raw
        $updatedVcpkgJson = $vcpkgJsonContent -replace '"version": "[0-9.]*"', "`"version`": `"$version`""
        Set-Content -Path $vcpkgJsonPath -Value $updatedVcpkgJson
        Write-Host "✅ Updated version in vcpkg.json" -ForegroundColor Green
    } else {
        Write-Host "❌ vcpkg.json not found at $vcpkgJsonPath" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "❌ Failed to update vcpkg.json: $_" -ForegroundColor Red
    exit 1
}

# Update versions/x-/xdelta.json
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
        Write-Host "❌ xdelta.json not found at $versionFilePath" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "❌ Failed to update xdelta.json: $_" -ForegroundColor Red
    exit 1
}

# Update baseline.json
try {
    $baselineJsonPath = "vcpkg-registry/versions/baseline.json"
    Write-Host "Updating $baselineJsonPath"

    if (Test-Path $baselineJsonPath) {
        $baselineJsonContent = Get-Content $baselineJsonPath -Raw
        $updatedBaselineJson = $baselineJsonContent -replace '"baseline": "[0-9.]*"', "`"baseline`": `"$version`""
        Set-Content -Path $baselineJsonPath -Value $updatedBaselineJson
        Write-Host "✅ Updated baseline in baseline.json" -ForegroundColor Green
    } else {
        Write-Host "❌ baseline.json not found at $baselineJsonPath" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "❌ Failed to update baseline.json: $_" -ForegroundColor Red
    exit 1
}

# Clean up
Remove-Item -Path $tempDir -Recurse -Force
Write-Host "Cleaned up temporary directory"

Write-Host "`n=== Update Summary ===" -ForegroundColor Cyan
Write-Host "✅ vcpkg registry update completed successfully" -ForegroundColor Green

# Return the SHA512 hash for use in GitHub Actions
return $sha512
