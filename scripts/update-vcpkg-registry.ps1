# Script to automatically update the vcpkg registry after a release
# Usage: .\scripts\update-vcpkg-registry.ps1 -ReleaseTag "v3.1.0" -BinaryZipUrl "https://github.com/loonghao/xdelta/releases/download/v3.1.0/xdelta-3.1.0-windows.zip"

param(
    [Parameter(Mandatory=$true)]
    [string]$ReleaseTag,

    [Parameter(Mandatory=$true)]
    [string]$BinaryZipUrl
)

# Extract version from release tag (remove 'v' prefix)
$version = $ReleaseTag -replace '^v', ''
Write-Host "Updating vcpkg registry for version $version" -ForegroundColor Cyan

# Create temp directory for downloads
$tempDir = Join-Path $env:TEMP "xdelta-vcpkg-update"
if (Test-Path $tempDir) {
    Remove-Item -Path $tempDir -Recurse -Force
}
New-Item -ItemType Directory -Path $tempDir | Out-Null

# Download the binary zip file
$zipPath = Join-Path $tempDir "xdelta-$version-windows.zip"
Write-Host "Downloading binary zip from $BinaryZipUrl" -ForegroundColor Cyan
Invoke-WebRequest -Uri $BinaryZipUrl -OutFile $zipPath

# Calculate SHA512 hash
$sha512 = (Get-FileHash -Path $zipPath -Algorithm SHA512).Hash.ToLower()
Write-Host "SHA512: $sha512" -ForegroundColor Green

# Update portfile.cmake
$portfilePath = "vcpkg-registry\ports\xdelta\portfile.cmake"
Write-Host "Updating $portfilePath" -ForegroundColor Cyan
$portfileContent = Get-Content $portfilePath -Raw
$updatedPortfileContent = $portfileContent -replace 'SHA512 "to-be-filled-after-release"', "SHA512 `"$sha512`""
$updatedPortfileContent = $updatedPortfileContent -replace 'SHA512 "[a-f0-9]+"', "SHA512 `"$sha512`""
Set-Content -Path $portfilePath -Value $updatedPortfileContent

# Update vcpkg.json version if needed
$vcpkgJsonPath = "vcpkg-registry\ports\xdelta\vcpkg.json"
Write-Host "Updating $vcpkgJsonPath" -ForegroundColor Cyan
$vcpkgJsonContent = Get-Content $vcpkgJsonPath -Raw
$updatedVcpkgJsonContent = $vcpkgJsonContent -replace '"version": "[0-9.]+"', "`"version`": `"$version`""
Set-Content -Path $vcpkgJsonPath -Value $updatedVcpkgJsonContent

# Update versions/x-/xdelta.json
$versionJsonPath = "vcpkg-registry\versions\x-\xdelta.json"
Write-Host "Updating $versionJsonPath" -ForegroundColor Cyan
$versionJsonContent = Get-Content $versionJsonPath -Raw
$updatedVersionJsonContent = $versionJsonContent -replace '"version": "[0-9.]+"', "`"version`": `"$version`""
$updatedVersionJsonContent = $updatedVersionJsonContent -replace '"git-tree": "to-be-filled-after-release"', "`"git-tree`": `"placeholder`""
$updatedVersionJsonContent = $updatedVersionJsonContent -replace '"git-tree": "[a-z0-9]+"', "`"git-tree`": `"placeholder`""
Set-Content -Path $versionJsonPath -Value $updatedVersionJsonContent

# Update baseline.json
$baselineJsonPath = "vcpkg-registry\versions\baseline.json"
Write-Host "Updating $baselineJsonPath" -ForegroundColor Cyan
$baselineJsonContent = Get-Content $baselineJsonPath -Raw
$updatedBaselineJsonContent = $baselineJsonContent -replace '"baseline": "[0-9.]+"', "`"baseline`": `"$version`""
Set-Content -Path $baselineJsonPath -Value $updatedBaselineJsonContent

Write-Host "vcpkg registry updated successfully for version $version" -ForegroundColor Green
Write-Host "SHA512: $sha512" -ForegroundColor Green

# Clean up
Remove-Item -Path $tempDir -Recurse -Force

# Return the SHA512 hash for use in CI
return $sha512
