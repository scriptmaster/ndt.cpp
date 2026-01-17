@echo off
REM Batch script to install GLFW on Windows
REM Tries winget first, then falls back to direct download

echo Setting up GLFW for Windows MinGW...

REM Create directories
if not exist "include" mkdir include
if not exist "lib" mkdir lib
if not exist "bin" mkdir bin

REM Check if GLFW is already installed
if exist "include\GLFW" (
    echo GLFW headers already found in include\GLFW
    exit /b 0
)

REM Try PowerShell script first (more reliable)
where powershell >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using PowerShell for installation...
    powershell -ExecutionPolicy Bypass -File "%~dp0setup_glfw.ps1"
    if %ERRORLEVEL% EQU 0 exit /b 0
)

REM Fallback: Try winget
where winget >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Checking winget for GLFW...
    winget search glfw >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo Note: Using direct download instead of winget for MinGW compatibility
    )
)

REM Direct download using PowerShell
echo Downloading GLFW 3.3.8 for MinGW64...
powershell -Command "& { $ErrorActionPreference = 'Stop'; $url = 'https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip'; $zip = 'glfw_temp.zip'; $extract = 'glfw_temp'; try { Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing; Add-Type -AssemblyName System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path $zip), $extract); $glfwDir = Get-ChildItem -Path $extract -Directory -Filter 'glfw-*' | Select-Object -First 1; if ($glfwDir -and (Test-Path (Join-Path $glfwDir.FullName 'include\GLFW'))) { Copy-Item -Path (Join-Path $glfwDir.FullName 'include\GLFW') -Destination 'include\' -Recurse -Force; $libPath = Join-Path $glfwDir.FullName 'lib-mingw-w64'; if (Test-Path $libPath) { Copy-Item -Path (Join-Path $libPath '*.a') -Destination 'lib\' -Force; Copy-Item -Path (Join-Path $libPath '*.dll') -Destination 'bin\' -Force -ErrorAction SilentlyContinue; } Remove-Item -Path $extract -Recurse -Force; Remove-Item -Path $zip -Force; Write-Host 'GLFW setup complete!' } else { throw 'Unexpected archive structure' } } catch { Write-Host \"Error: $_\"; Write-Host \"Please download manually from: https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip\"; exit 1 }"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Installation failed. Please run setup_glfw.ps1 manually or download from:
    echo https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip
    exit /b 1
)

echo GLFW setup complete!
echo Headers: .\include\GLFW
echo Libraries: .\lib
echo DLLs: .\bin
