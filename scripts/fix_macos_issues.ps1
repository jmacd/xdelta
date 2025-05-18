# PowerShell script to fix macOS-specific issues in xdelta3 code
# This script addresses C++11 literal suffix problems and other warnings

# 1. Fix the C++11 literal suffix issue in xdelta3.c line 4432
Write-Host "Fixing C++11 literal suffix issue in xdelta3.c..."
$file = "xdelta3\xdelta3.c"
$content = Get-Content $file -Raw -ErrorAction SilentlyContinue

if ($content) {
    # Fix the %"Z"d format string
    $oldString = 'IF_DEBUG2 (DP(RINT "[srcwin_move_point] continue (end-of-block): %"Z"d\n", blkpos));'
    $newString = 'IF_DEBUG2 (DP(RINT "[srcwin_move_point] continue (end-of-block): %" Z "d\n", blkpos));'

    if ($content -match [regex]::Escape($oldString)) {
        $content = $content -replace [regex]::Escape($oldString), $newString
        Set-Content -Path $file -Value $content
        Write-Host "  - Fixed C++11 literal suffix issue in xdelta3.c"
    } else {
        Write-Host "  - Could not find the exact string to replace in xdelta3.c"
    }
} else {
    Write-Host "  - Could not read xdelta3.c"
}

# 2. Fix the logical operator issue in xdelta3-fgk.h line 266
Write-Host "Fixing logical operator issue in xdelta3-fgk.h..."
$file = "xdelta3\xdelta3-fgk.h"
$content = Get-Content $file -Raw -ErrorAction SilentlyContinue

if ($content) {
    # Fix the logical && operator that should be bitwise &
    $oldString = 'h->coded_bits[h->coded_depth++] = (shift & where) && 1;'
    $newString = 'h->coded_bits[h->coded_depth++] = (shift & where) & 1;'

    if ($content -match [regex]::Escape($oldString)) {
        $content = $content -replace [regex]::Escape($oldString), $newString
        Set-Content -Path $file -Value $content
        Write-Host "  - Fixed logical operator issue in xdelta3-fgk.h"
    } else {
        Write-Host "  - Could not find the exact string to replace in xdelta3-fgk.h"
    }
} else {
    Write-Host "  - Could not read xdelta3-fgk.h"
}

# 3. Fix unused parameter warnings by adding appropriate annotations
Write-Host "Fixing unused parameter warnings..."

# Fix unused parameter 'scksum' in xdelta3.c
$file = "xdelta3\xdelta3.c"
$content = Get-Content $file -Raw -ErrorAction SilentlyContinue

if ($content) {
    # Add XD3_UNUSED macro to the unused parameter
    $oldString = 'xd3_smatch (xd3_stream *stream,
		    usize_t base,
		    usize_t scksum,
		    usize_t *match_offset)'
    $newString = 'xd3_smatch (xd3_stream *stream,
		    usize_t base,
		    usize_t scksum XD3_UNUSED,
		    usize_t *match_offset)'

    if ($content -match [regex]::Escape($oldString)) {
        $content = $content -replace [regex]::Escape($oldString), $newString
        Set-Content -Path $file -Value $content
        Write-Host "  - Fixed unused parameter 'scksum' in xdelta3.c"
    } else {
        Write-Host "  - Could not find the exact string to replace for 'scksum' in xdelta3.c"
    }
} else {
    Write-Host "  - Could not read xdelta3.c"
}

# Fix unused parameters in xdelta3-fgk.h
$file = "xdelta3\xdelta3-fgk.h"
$content = Get-Content $file -Raw -ErrorAction SilentlyContinue

if ($content) {
    # Fix unused parameter 'stream' and 'is_encode'
    $oldString = 'static int fgk_init (xd3_stream *stream, fgk_stream *h, int is_encode)'
    $newString = 'static int fgk_init (xd3_stream *stream XD3_UNUSED, fgk_stream *h, int is_encode XD3_UNUSED)'

    if ($content -match [regex]::Escape($oldString)) {
        $content = $content -replace [regex]::Escape($oldString), $newString

        # Fix unused parameter 'h' in fgk_move_right
        $oldString2 = 'static void fgk_move_right (fgk_stream *h, fgk_node *move_fwd)'
        $newString2 = 'static void fgk_move_right (fgk_stream *h XD3_UNUSED, fgk_node *move_fwd)'

        if ($content -match [regex]::Escape($oldString2)) {
            $content = $content -replace [regex]::Escape($oldString2), $newString2
            Set-Content -Path $file -Value $content
            Write-Host "  - Fixed unused parameters in xdelta3-fgk.h"
        } else {
            Write-Host "  - Could not find the exact string to replace for 'h' in xdelta3-fgk.h"
            Set-Content -Path $file -Value $content
        }
    } else {
        Write-Host "  - Could not find the exact string to replace for 'stream' and 'is_encode' in xdelta3-fgk.h"
    }
} else {
    Write-Host "  - Could not read xdelta3-fgk.h"
}

Write-Host "macOS-specific fixes completed"
