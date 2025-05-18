#!/usr/bin/env pwsh
# Script to update vcpkg registry with new release information
# Usage: ./scripts/update-vcpkg-registry.ps1 -ReleaseTag v3.1.0 -BinaryZipUrl https://github.com/loonghao/xdelta/releases/download/v3.1.0/xdelta-3.1.0-windows.zip

param(
    [Parameter(Mandatory=$true)]
    [string]$ReleaseTag,

    [Parameter(Mandatory=$true)]
    [string]$BinaryZipUrl
)

# Extract version from tag
$version = $ReleaseTag -replace '^v', ''
Write-Host "Updating vcpkg registry for version $version"

# Temporary directory for downloads
# Use a fallback if TEMP environment variable is not set
if ($env:TEMP) {
    $tempDir = Join-Path $env:TEMP "xdelta-vcpkg-update"
} else {
    # Fallback to current directory if TEMP is not set
    $tempDir = Join-Path (Get-Location) "temp-xdelta-vcpkg-update"
    Write-Host "TEMP environment variable not set, using current directory: $tempDir"
}
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

# Download the binary package to calculate SHA512
$zipPath = Join-Path $tempDir "xdelta-$version-windows.zip"
Write-Host "Downloading binary package from $BinaryZipUrl"
Invoke-WebRequest -Uri $BinaryZipUrl -OutFile $zipPath

# Calculate SHA512 hash
$sha512 = (Get-FileHash -Path $zipPath -Algorithm SHA512).Hash.ToLower()
Write-Host "SHA512: $sha512"

# Update portfile.cmake
$portfilePath = "vcpkg-registry/ports/xdelta/portfile.cmake"
Write-Host "Updating $portfilePath"
$portfileContent = Get-Content $portfilePath -Raw
$updatedContent = $portfileContent -replace 'SHA512 "to-be-filled-after-release"', "SHA512 `"$sha512`""
$updatedContent = $updatedContent -replace 'SHA512 "将在发布后填写实际的哈希值"', "SHA512 `"$sha512`""
Set-Content -Path $portfilePath -Value $updatedContent

# Update vcpkg.json if needed
$vcpkgJsonPath = "vcpkg-registry/ports/xdelta/vcpkg.json"
Write-Host "Updating $vcpkgJsonPath"
$vcpkgJsonContent = Get-Content $vcpkgJsonPath -Raw
$updatedVcpkgJson = $vcpkgJsonContent -replace '"version": "[0-9.]*"', "`"version`": `"$version`""
Set-Content -Path $vcpkgJsonPath -Value $updatedVcpkgJson

# Update versions/x-/xdelta.json
$versionFilePath = "vcpkg-registry/versions/x-/xdelta.json"
Write-Host "Updating $versionFilePath"
$versionFileContent = Get-Content $versionFilePath -Raw

# Get the current commit hash to use as git-tree
$gitTree = git rev-parse HEAD
Write-Host "Using git-tree: $gitTree"

$updatedVersionContent = $versionFileContent -replace '"git-tree": "to-be-filled-after-release"', "`"git-tree`": `"$gitTree`""
$updatedVersionContent = $updatedVersionContent -replace '"version": "[0-9.]*"', "`"version`": `"$version`""
Set-Content -Path $versionFilePath -Value $updatedVersionContent

# Update baseline.json
$baselineJsonPath = "vcpkg-registry/versions/baseline.json"
Write-Host "Updating $baselineJsonPath"
$baselineJsonContent = Get-Content $baselineJsonPath -Raw
$updatedBaselineJson = $baselineJsonContent -replace '"baseline": "[0-9.]*"', "`"baseline`": `"$version`""
Set-Content -Path $baselineJsonPath -Value $updatedBaselineJson

# Clean up
Remove-Item -Path $tempDir -Recurse -Force

# Return the SHA512 hash for use in GitHub Actions
return $sha512
