# PowerShell script to fix C++11 literal suffix format strings in xdelta3.c
# This script specifically targets the format string issues in xdelta3.c

# Define the file to process
$file = "..\xdelta3\xdelta3.c"

# Read the file content
$content = Get-Content $file -Raw -ErrorAction SilentlyContinue

if ($content) {
    Write-Host "Processing $file"
    $originalContent = $content
    $changed = $false

    # Define patterns to search for and their replacements
    $patterns = @(
        # Format strings with W
        @{
            search = '%"W"u'
            replace = '%" W "u'
        },
        @{
            search = '%" W"u'
            replace = '%" W "u'
        },
        @{
            search = '%"W" u'
            replace = '%" W "u'
        },
        @{
            search = '%W"u'
            replace = '%" W "u'
        },
        @{
            search = '%Wu'
            replace = '% W u'
        },
        # Format strings with Q
        @{
            search = '%"Q"u'
            replace = '%" Q "u'
        },
        @{
            search = '%" Q"u'
            replace = '%" Q "u'
        },
        @{
            search = '%"Q" u'
            replace = '%" Q "u'
        },
        @{
            search = '%Q"u'
            replace = '%" Q "u'
        },
        @{
            search = '%Qu'
            replace = '% Q u'
        },
        # Format strings with O
        @{
            search = '%"O"u'
            replace = '%" O "u'
        },
        @{
            search = '%" O"u'
            replace = '%" O "u'
        },
        @{
            search = '%"O" u'
            replace = '%" O "u'
        },
        @{
            search = '%O"u'
            replace = '%" O "u'
        },
        @{
            search = '%Ou'
            replace = '% O u'
        }
    )

    # Process each pattern
    foreach ($pattern in $patterns) {
        if ($content -match $pattern.search) {
            Write-Host "  - Replacing $($pattern.search) with $($pattern.replace)"
            $content = $content -replace [regex]::Escape($pattern.search), $pattern.replace
            $changed = $true
        }
    }

    # Save the modified content back to the file if changes were made
    if ($changed) {
        Set-Content -Path $file -Value $content
        Write-Host "  - File updated"
        
        # Compare with original to show what changed
        $diffCount = (Compare-Object ($originalContent -split "`n") ($content -split "`n")).Count
        Write-Host "  - $diffCount lines changed"
    } else {
        Write-Host "  - No changes needed"
    }
} else {
    Write-Host "Could not read file: $file"
}

Write-Host "Format string fixes completed for xdelta3.c"
