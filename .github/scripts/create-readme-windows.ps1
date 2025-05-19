#!/usr/bin/env pwsh
# Script to create README.txt for Windows artifacts
# Usage: ./create-readme-windows.ps1 -ArtifactsDir <path_to_artifacts_dir>

param (
    [Parameter(Mandatory=$true)]
    [string]$ArtifactsDir
)

# Ensure artifacts directory exists
if (-not (Test-Path $ArtifactsDir)) {
    Write-Host "Creating artifacts directory: $ArtifactsDir"
    New-Item -Path $ArtifactsDir -ItemType Directory -Force | Out-Null
}

# Create README path
$readmePath = Join-Path -Path $ArtifactsDir -ChildPath "README.txt"

Write-Host "Creating README file at: $readmePath"

# Create the README content
$readmeContent = @"
Xdelta3 Windows Binary
======================

This package contains the xdelta3 command-line utility for Windows.

Command Line Syntax
-------------------

make patch:

  xdelta3.exe -e -s old_file new_file delta_file

apply patch:

  xdelta3.exe -d -s old_file delta_file decoded_new_file

standard options:
   -0 .. -9     compression level
   -c           use stdout
   -d           decompress
   -e           compress
   -f           force (overwrite, ignore trailing garbage)
   -h           show help
   -q           be quiet
   -v           be verbose (max 2)
   -V           show version

For full documentation, run: xdelta3.exe --help
"@

try {
    # Write the README content
    Set-Content -Path $readmePath -Value $readmeContent -ErrorAction Stop
    Write-Host "README.txt created successfully"
    
    # Verify the file was created
    if (Test-Path $readmePath) {
        Write-Host "README.txt verified at: $readmePath"
        Get-Content $readmePath | Select-Object -First 3 | ForEach-Object { Write-Host "  $_" }
    } else {
        throw "README.txt not found at: $readmePath after creation"
    }
} catch {
    Write-Host "Error creating README: $_"
    Write-Host "Stack trace: $($_.ScriptStackTrace)"
    Write-Host "WARNING: Failed to create README, trying alternative method..."
    
    try {
        # Try alternative method with Out-File
        $readmeContent | Out-File -FilePath $readmePath -Encoding utf8
        Write-Host "README.txt created with alternative method"
    } catch {
        Write-Host "Error with alternative method: $_"
        Write-Host "Creating README with simplified approach..."
        
        # Simplest approach with individual lines
        "Xdelta3 Windows Binary" | Set-Content -Path $readmePath
        "======================" | Add-Content -Path $readmePath
        "" | Add-Content -Path $readmePath
        "This package contains the xdelta3 command-line utility for Windows." | Add-Content -Path $readmePath
        "" | Add-Content -Path $readmePath
        "For usage information, run: xdelta3.exe --help" | Add-Content -Path $readmePath
        
        Write-Host "Simplified README created"
    }
}
