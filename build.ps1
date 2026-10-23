# build.ps1 - Build BLACK Browser on Windows
param(
    [string]$QtPath = "C:\Qt\6.7.2\msvc2022_64",  # CHANGE THIS to your Qt path
    [string]$Config = "Release"
)

Write-Host "=== BLACK Browser Build Script ===" -ForegroundColor Cyan

# Check prerequisites
$errors = @()
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $errors += "CMake not in PATH" }
if (-not (Get-Command msbuild -ErrorAction SilentlyContinue)) { $errors += "MSBuild not in PATH (install VS 2022 with C++ workload)" }
if (-not (Test-Path "$QtPath\bin\qt-cmake.exe")) { $errors += "Qt not found at $QtPath`nInstall Qt 6 MSVC 64-bit and update `$QtPath in script" }

if ($errors) {
    Write-Host "Missing prerequisites:" -ForegroundColor Red
    $errors | ForEach-Object { Write-Host "  ✗ $_" -ForegroundColor Red }
    Write-Host "`nFix:`n  1. Install Visual Studio 2022 Community + 'Desktop development with C++'`n  2. Install Qt 6 (MSVC 64-bit) from qt.io`n  3. Install CMake from cmake.org`n  4. Edit `$QtPath in this script to match your Qt version" -ForegroundColor Yellow
    exit 1
}

# Configure
Write-Host "`nConfiguring with CMake..." -ForegroundColor Yellow
mkdir build -ErrorAction SilentlyContinue
cd build
cmake .. -DCMAKE_PREFIX_PATH="$QtPath" -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE) { Write-Host "CMake configure failed" -ForegroundColor Red; exit 1 }

# Build
Write-Host "`nBuilding ($Config)..." -ForegroundColor Yellow
cmake --build . --config $Config --parallel
if ($LASTEXITCODE) { Write-Host "Build failed" -ForegroundColor Red; exit 1 }

$exe = ".\$Config\BLACK.exe"
if (Test-Path $exe) {
    Write-Host "`n✅ Success! Executable at:" -ForegroundColor Green
    Write-Host "  $(Resolve-Path $exe)" -ForegroundColor Green
    Write-Host "`nRun it: .\$Config\BLACK.exe" -ForegroundColor Cyan
} else {
    Write-Host "`n❌ Build completed but exe not found at $exe" -ForegroundColor Red
}