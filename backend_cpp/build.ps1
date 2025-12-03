param(
    [string]$Generator = "Visual Studio 17 2022",
    [string]$BuildType = "Release"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Push-Location $scriptDir

if (-Not (Test-Path build)) { New-Item -ItemType Directory -Path build | Out-Null }

cmake -S . -B build -G "$Generator" -A x64
cmake --build build --config $BuildType

$exe = "build\$BuildType\minic_backend.exe"
if (Test-Path $exe) {
    Copy-Item $exe -Destination ".\minic_backend.exe" -Force
    Write-Host "Built and copied to .\minic_backend.exe"
} else {
    Write-Host "Build completed but exe not found at $exe"
}

Pop-Location
