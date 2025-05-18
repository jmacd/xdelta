# PowerShell script to fix C++11 literal suffix format strings in all C/C++ files
# This script finds and replaces problematic format strings in the entire codebase

# Define patterns to search for and their replacements
$patterns = @(
    # Common format string patterns with quotes but no spaces
    @{
        search = '%"Q"u'
        replace = '%" Q "u'
    },
    @{
        search = '%"W"u'
        replace = '%" W "u'
    },
    @{
        search = '%"O"u'
        replace = '%" O "u'
    },
    # Patterns with no spaces between % and letter
    @{
        search = '%Q"u'
        replace = '% Q "u'
    },
    @{
        search = '%W"u'
        replace = '% W "u'
    },
    @{
        search = '%O"u'
        replace = '% O "u'
    },
    # Patterns with quotes but no spaces
    @{
        search = '"%Q"'
        replace = '"% Q "'
    },
    @{
        search = '"%W"'
        replace = '"% W "'
    },
    @{
        search = '"%O"'
        replace = '"% O "'
    },
    # Additional patterns that might cause issues
    @{
        search = '%"Q"d'
        replace = '%" Q "d'
    },
    @{
        search = '%"W"d'
        replace = '%" W "d'
    },
    @{
        search = '%"O"d'
        replace = '%" O "d'
    },
    # Patterns with no spaces and different formats
    @{
        search = '%"Q"x'
        replace = '%" Q "x'
    },
    @{
        search = '%"W"x'
        replace = '%" W "x'
    },
    @{
        search = '%"O"x'
        replace = '%" O "x'
    },
    # Patterns with different spacing
    @{
        search = '%"Q" u'
        replace = '%" Q "u'
    },
    @{
        search = '%"W" u'
        replace = '%" W "u'
    },
    @{
        search = '%"O" u'
        replace = '%" O "u'
    },
    # Patterns with different spacing
    @{
        search = '% "Q"u'
        replace = '%" Q "u'
    },
    @{
        search = '% "W"u'
        replace = '%" W "u'
    },
    @{
        search = '% "O"u'
        replace = '%" O "u'
    },
    # Patterns without quotes
    @{
        search = '%Wu'
        replace = '% W u'
    },
    @{
        search = '%Qu'
        replace = '% Q u'
    },
    @{
        search = '%Ou'
        replace = '% O u'
    },
    # Additional formats
    @{
        search = '%"Q"s'
        replace = '%" Q "s'
    },
    @{
        search = '%"W"s'
        replace = '%" W "s'
    },
    @{
        search = '%"O"s'
        replace = '%" O "s'
    }
)

# Get all C/C++ files in the repository
$files = Get-ChildItem -Path . -Recurse -Include *.c,*.cpp,*.h,*.hpp

# Process each file
foreach ($file in $files) {
    Write-Host "Processing $($file.FullName)"
    $content = Get-Content $file.FullName -Raw -ErrorAction SilentlyContinue
    
    if ($content) {
        $changed = $false
        
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
            Set-Content -Path $file.FullName -Value $content
            Write-Host "  - File updated"
        } else {
            Write-Host "  - No changes needed"
        }
    } else {
        Write-Host "  - Could not read file content"
    }
}

Write-Host "Format string fixes completed for all C/C++ files"
