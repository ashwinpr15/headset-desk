param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$Destination,
    [string]$Version = 'v0.1.1-diagnostics.1'
)
$ErrorActionPreference = 'Stop'
$sourceRoot = Split-Path $PSScriptRoot -Parent
$exePath = (Resolve-Path -LiteralPath $Executable).Path
$destinationPath = [IO.Path]::GetFullPath($Destination)
$bundle = Join-Path $destinationPath "headset-desk-$Version-windows-x64"
if (Test-Path -LiteralPath $bundle) { throw 'Bundle directory already exists; choose a new destination' }
$archive = "$bundle.zip"
if (Test-Path -LiteralPath $archive) { throw 'Archive already exists; choose a new destination' }
New-Item -ItemType Directory -Path $bundle -Force | Out-Null
Copy-Item -LiteralPath $exePath -Destination (Join-Path $bundle 'headset-desk-diagnostics.exe')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE') -Destination (Join-Path $bundle 'LICENSE-Headset-Desk.txt')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'vendor/sony-device-center/LICENSE') -Destination (Join-Path $bundle 'LICENSE-Sony-Device-Center.txt')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'runtime-notices') -Destination $bundle -Recurse
Copy-Item -LiteralPath (Join-Path $sourceRoot "docs/releases/$Version.md") -Destination (Join-Path $bundle 'READ-ME-FIRST.md')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'docs/hardware-validation.md') -Destination $bundle
Copy-Item -LiteralPath (Join-Path $sourceRoot 'docs/build-validation.md') -Destination $bundle
Compress-Archive -LiteralPath $bundle -DestinationPath $archive
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$archive.sha256", "$hash  $([IO.Path]::GetFileName($archive))`n")
Write-Output $archive
Write-Output "SHA256 $hash"
