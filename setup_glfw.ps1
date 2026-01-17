# PowerShell script to install GLFW on Windows
# Tries winget first, then falls back to direct download

$ErrorActionPreference = "Stop"

Write-Host "Setting up GLFW for Windows MinGW..." -ForegroundColor Cyan

# Create directories
if (-not (Test-Path "include")) { New-Item -ItemType Directory -Path "include" | Out-Null }
if (-not (Test-Path "lib")) { New-Item -ItemType Directory -Path "lib" | Out-Null }
if (-not (Test-Path "bin")) { New-Item -ItemType Directory -Path "bin" | Out-Null }

# Check if GLFW is already installed
if (Test-Path "include\GLFW") {
    Write-Host "GLFW headers already found in include\GLFW" -ForegroundColor Green
    exit 0
}

# Try winget first (though GLFW may not be in winget)
Write-Host "Checking for winget..." -ForegroundColor Yellow
$wingetPath = Get-Command winget -ErrorAction SilentlyContinue

if ($wingetPath) {
    Write-Host "winget found. Checking for GLFW package..." -ForegroundColor Yellow
    # Note: GLFW might not be available in winget, but we check anyway
    $glfwCheck = winget search glfw 2>$null | Select-String -Pattern "glfw" -Quiet
    if ($glfwCheck) {
        Write-Host "GLFW found in winget. Attempting installation..." -ForegroundColor Yellow
        # Winget might install to Program Files, so we skip this and use direct download
        Write-Host "Note: winget GLFW may not be compatible with MinGW. Using direct download instead." -ForegroundColor Yellow
    }
}

# Direct download from GitHub
Write-Host "Downloading GLFW 3.3.8 for MinGW64 from GitHub..." -ForegroundColor Cyan
$glfwUrl = "https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip"
$zipFile = "glfw_temp.zip"
$extractDir = "glfw_temp"

try {
    # Download using PowerShell's Invoke-WebRequest
    Write-Host "Downloading from $glfwUrl..." -ForegroundColor Yellow
    Invoke-WebRequest -Uri $glfwUrl -OutFile $zipFile -UseBasicParsing
    
    Write-Host "Extracting GLFW..." -ForegroundColor Yellow
    
    # Extract using .NET ZipFile class
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path $zipFile), $extractDir)
    
    # Find the extracted directory
    $glfwDir = Get-ChildItem -Path $extractDir -Directory -Filter "glfw-*" | Select-Object -First 1
    
    if ($glfwDir -and (Test-Path (Join-Path $glfwDir.FullName "include\GLFW"))) {
        # Copy headers
        Copy-Item -Path (Join-Path $glfwDir.FullName "include\GLFW") -Destination "include\" -Recurse -Force
        Write-Host "Copied headers to include\GLFW" -ForegroundColor Green
        
        # Copy libraries
        $libPath = Join-Path $glfwDir.FullName "lib-mingw-w64"
        if (Test-Path $libPath) {
            Copy-Item -Path (Join-Path $libPath "*.a") -Destination "lib\" -Force
            Copy-Item -Path (Join-Path $libPath "*.dll") -Destination "bin\" -Force -ErrorAction SilentlyContinue
            Write-Host "Copied libraries to lib\" -ForegroundColor Green
            Write-Host "Copied DLLs to bin\" -ForegroundColor Green
        }
        
        Write-Host "`nGLFW setup complete!" -ForegroundColor Green
        Write-Host "Headers: .\include\GLFW" -ForegroundColor Cyan
        Write-Host "Libraries: .\lib" -ForegroundColor Cyan
        Write-Host "DLLs: .\bin" -ForegroundColor Cyan
    } else {
        throw "Unexpected archive structure"
    }
    
    # Cleanup
    Remove-Item -Path $extractDir -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path $zipFile -Force -ErrorAction SilentlyContinue
    
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "`nManual installation:" -ForegroundColor Yellow
    Write-Host "1. Download GLFW from: https://www.glfw.org/download.html" -ForegroundColor Yellow
    Write-Host "2. Download: $glfwUrl" -ForegroundColor Yellow
    Write-Host "3. Extract the MinGW64 pre-built binaries" -ForegroundColor Yellow
    Write-Host "4. Copy include\GLFW to .\include\GLFW" -ForegroundColor Yellow
    Write-Host "5. Copy lib-mingw-w64\*.a to .\lib\" -ForegroundColor Yellow
    Write-Host "6. Copy lib-mingw-w64\*.dll to .\bin\" -ForegroundColor Yellow
    
    if (Test-Path $zipFile) { Remove-Item -Path $zipFile -Force -ErrorAction SilentlyContinue }
    if (Test-Path $extractDir) { Remove-Item -Path $extractDir -Recurse -Force -ErrorAction SilentlyContinue }
    exit 1
}
