param(
  [Parameter(Mandatory = $true)]
  [string]$Xdelta
)

$ErrorActionPreference = "Stop"
$unicodeName = "Restaura$([char]0x00E7)$([char]0x00E3)o-$([char]0x6F22)$([char]0x5B57)"
$testDir = Join-Path ([IO.Path]::GetTempPath()) ("xdelta3-unicode-" + [guid]::NewGuid())

try {
  $sourceDir = Join-Path $testDir "$unicodeName-source"
  $targetDir = Join-Path $testDir "$unicodeName-target"
  $patchDir = Join-Path $testDir "$unicodeName-patch"
  [IO.Directory]::CreateDirectory($sourceDir) | Out-Null
  [IO.Directory]::CreateDirectory($targetDir) | Out-Null
  [IO.Directory]::CreateDirectory($patchDir) | Out-Null

  $source = Join-Path $sourceDir "$unicodeName.txt"
  $target = Join-Path $targetDir "$unicodeName.txt"
  $patch = Join-Path $patchDir "$unicodeName.vcdiff"
  $decoded = Join-Path $targetDir "$unicodeName-decoded.txt"
  [IO.File]::WriteAllBytes($source, [Text.Encoding]::UTF8.GetBytes("before`n"))
  [IO.File]::WriteAllBytes($target, [Text.Encoding]::UTF8.GetBytes("before`nafter`n"))

  & $Xdelta -e -s $source $target $patch
  if ($LASTEXITCODE -ne 0) {
    throw "Unicode encode failed with exit code $LASTEXITCODE"
  }
  & $Xdelta -d -s $source $patch $decoded
  if ($LASTEXITCODE -ne 0) {
    throw "Unicode decode failed with exit code $LASTEXITCODE"
  }

  $expected = (Get-FileHash -Algorithm SHA256 $target).Hash
  $actual = (Get-FileHash -Algorithm SHA256 $decoded).Hash
  if ($actual -ne $expected) {
    throw "Decoded Unicode-path file does not match the target"
  }
}
finally {
  if (Test-Path -LiteralPath $testDir) {
    Remove-Item -LiteralPath $testDir -Recurse -Force
  }
}
