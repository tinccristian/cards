$ErrorActionPreference = "Stop"

$root    = $PSScriptRoot
$build   = "$root\build"
$out     = "$root\dist\MedicalDeckbuilder"
$zipPath = "$root\deckbuilder.zip"

Write-Host "Building release..." -ForegroundColor Cyan
cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed."; exit 1 }

Write-Host "Packaging..." -ForegroundColor Cyan
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Force $out | Out-Null

Copy-Item "$build\medical_deckbuilder.exe" "$out\MedicalDeckbuilder.exe"
Copy-Item -Recurse "$root\assets" "$out\assets"

if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path $out -DestinationPath $zipPath

Write-Host "Done: $zipPath" -ForegroundColor Green
