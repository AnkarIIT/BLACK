# Packaging and Deployment Automation Script
Param(
    [string]$QtPath = "C:\Codes\browser\Qt\6.8.0\msvc2022_64",
    [string]$CertPath = "",          # Path to .pfx certificate file
    [string]$CertPassword = "",      # Certificate password (if required)
    [string]$TimestampUrl = "http://timestamp.digicert.com",  # RFC 3161 timestamp server
    [switch]$SkipSigning             # Skip code signing step
)

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Building BLACK Release Package... " -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

# Set environment PATH
$env:PATH = "$QtPath\bin;$env:PATH"

# Helper: find signtool.exe
function Find-SignTool {
    $paths = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe",
        "${env:ProgramFiles}\Windows Kits\10\bin\*\x64\signtool.exe",
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.*\x64\signtool.exe"
    )
    foreach ($pattern in $paths) {
        $resolved = Resolve-Path $pattern -ErrorAction SilentlyContinue
        if ($resolved) { return $resolved.Path }
    }
    return $null
}

# 1. Build project via CMake
Write-Host "`n[1/4] Compiling Release executable..." -ForegroundColor Yellow
cmake -S . -B build -DCMAKE_PREFIX_PATH="$QtPath"
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
Write-Host "`n[2/4] Bundling Qt DLLs and WebEngine assets via windeployqt..." -ForegroundColor Yellow
$winDeploy = "$QtPath\bin\windeployqt.exe"
& $winDeploy --release .\build\Release\BLACK.exe

# 3. Code signing (if certificate provided)
if (-not $SkipSigning -and $CertPath) {
    Write-Host "`n[3/4] Code signing executables and installer..." -ForegroundColor Yellow
    $signTool = Find-SignTool
    if (-not $signTool) {
        Write-Warning "signtool.exe not found in Windows SDK. Skipping code signing."
    } else {
        Write-Host "Using signtool: $signTool" -ForegroundColor Gray

        $signArgs = @(
            "sign",
            "/f", $CertPath,
            "/t", $TimestampUrl,
            "/fd", "sha256",
            "/v"
        )
        if ($CertPassword) {
            $signArgs += "/p", $CertPassword
        }

        # Sign all .exe and .dll in build/Release
        $files = Get-ChildItem .\build\Release -Recurse -Include *.exe, *.dll
        foreach ($file in $files) {
            Write-Host "Signing $($file.FullName)..." -ForegroundColor Gray
            & $signTool $signArgs $file.FullName
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Failed to sign $($file.FullName)"
            }
        }
    }
} elseif ($SkipSigning) {
    Write-Host "`n[3/4] Skipping code signing (--SkipSigning specified)..." -ForegroundColor Yellow
} else {
    Write-Host "`n[3/4] Skipping code signing (no certificate provided)..." -ForegroundColor Yellow
}

# 4. Run Inno Setup to create installer
Write-Host "`n[4/4] Creating Inno Setup installer..." -ForegroundColor Yellow
$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    $iscc = "C:\Program Files\Inno Setup 6\ISCC.exe"
}
if (Test-Path $iscc) {
    & $iscc installer.iss
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Inno Setup compilation failed."
        exit 1
    }

    # Sign the installer if certificate provided
    if (-not $SkipSigning -and $CertPath -and $signTool) {
        $installerPath = ".\Output\BLACK_Setup_v1.0.exe"
        if (Test-Path $installerPath) {
            Write-Host "Signing installer..." -ForegroundColor Gray
            & $signTool $signArgs $installerPath
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Failed to sign installer"
            }
        }
    }
} else {
    Write-Warning "Inno Setup not found. Skipping installer creation."
}

# 5. Verify deployment output
Write-Host "`n[5/5] Verifying release package..." -ForegroundColor Yellow
if (Test-Path ".\build\Release\Qt6WebEngineCore.dll") {
    Write-Host "`n[SUCCESS] Package bundling complete! Output ready at: .\build\Release" -ForegroundColor Green
    if (Test-Path ".\Output\BLACK_Setup_v1.0.exe") {
        Write-Host "Installer created at: .\Output\BLACK_Setup_v1.0.exe" -ForegroundColor Green
    }
} else {
    Write-Warning "windeployqt output check failed."
}