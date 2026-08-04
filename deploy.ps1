# Packaging and Deployment Automation Script
Param(
    [string]$QtPath = "C:\Codes\browser\Qt\6.8.0\msvc2022_64"
)

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Building Safari Release Package... " -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

# Set environment PATH
$env:PATH = "$QtPath\bin;$env:PATH"

# 1. Build project via CMake
Write-Host "`n[1/3] Compiling Release executable..." -ForegroundColor Yellow
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="$QtPath"
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
    exit 1
}

cmake --build build --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit 1
}

# 2. Run windeployqt
Write-Host "`n[2/3] Bundling Qt DLLs and WebEngine assets via windeployqt..." -ForegroundColor Yellow
$winDeploy = "$QtPath\bin\windeployqt.exe"
& $winDeploy --webengine --release .\build\Release\BLACK.exe

# 3. Check deployment output
Write-Host "`n[3/3] Verifying release package directory..." -ForegroundColor Yellow
if (Test-Path ".\build\Release\Qt6WebEngineCore.dll") {
    Write-Host "`n[SUCCESS] Package bundling complete! Output ready at: .\build\Release" -ForegroundColor Green
} else {
    Write-Warning "windeployqt output check failed."
}
